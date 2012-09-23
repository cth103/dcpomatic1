/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

/** @file src/server.cc
 *  @brief Class to describe a server to which we can send
 *  encoding work, and a class to implement such a server.
 */

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "server.h"
#include "util.h"
#include "scaler.h"
#include "image.h"
#include "dcp_video_frame.h"
#include "config.h"

using namespace std;
using namespace boost;

/** Create a server description from a string of metadata returned from as_metadata().
 *  @param v Metadata.
 *  @return ServerDescription, or 0.
 */
ServerDescription *
ServerDescription::create_from_metadata (string v)
{
	vector<string> b;
	split (b, v, is_any_of (" "));

	if (b.size() != 2) {
		return 0;
	}

	return new ServerDescription (b[0], atoi (b[1].c_str ()));
}

/** @return Description of this server as text */
string
ServerDescription::as_metadata () const
{
	stringstream s;
	s << _host_name << " " << _threads;
	return s.str ();
}

Server::Server (Log* log)
	: _log (log)
{

}

int
Server::process (shared_ptr<asio::ip::tcp::socket> socket)
{
	DeadlineWrapper wrapper (_io_service);
	wrapper.set_socket (socket);
	
	char buffer[128];
	wrapper.read_indefinite ((uint8_t *) buffer, sizeof (buffer), 30);
	wrapper.consume (strlen (buffer) + 1);
	
	stringstream s (buffer);
	
	string command;
	s >> command;
	if (command != "encode") {
		return -1;
	}
	
	Size in_size;
	int pixel_format_int;
	Size out_size;
	int padding;
	string scaler_id;
	int frame;
	float frames_per_second;
	string post_process;
	int colour_lut_index;
	int j2k_bandwidth;
	
	s >> in_size.width >> in_size.height
	  >> pixel_format_int
	  >> out_size.width >> out_size.height
	  >> padding
	  >> scaler_id
	  >> frame
	  >> frames_per_second
	  >> post_process
	  >> colour_lut_index
	  >> j2k_bandwidth;
	
	PixelFormat pixel_format = (PixelFormat) pixel_format_int;
	Scaler const * scaler = Scaler::from_id (scaler_id);
	if (post_process == "none") {
		post_process = "";
	}
	
	shared_ptr<SimpleImage> image (new SimpleImage (pixel_format, in_size));
	
	for (int i = 0; i < image->components(); ++i) {
		int line_size;
		s >> line_size;
		image->set_line_size (i, line_size);
	}
	
	for (int i = 0; i < image->components(); ++i) {
		wrapper.read_definite_and_consume (image->data()[i], image->line_size()[i] * image->lines(i), 30);
	}
	
#ifdef DEBUG_HASH
	image->hash ("Image for encoding (as received by server)");
#endif		
	
	DCPVideoFrame dcp_video_frame (image, out_size, padding, scaler, frame, frames_per_second, post_process, colour_lut_index, j2k_bandwidth, _log);
	shared_ptr<EncodedData> encoded = dcp_video_frame.encode_locally ();
	encoded->send (socket);

#ifdef DEBUG_HASH
	encoded->hash ("Encoded image (as made by server and as sent back)");
#endif		
	
	return frame;
}

void
Server::worker_thread ()
{
	while (1) {
		mutex::scoped_lock lock (_worker_mutex);
		while (_queue.empty ()) {
			_worker_condition.wait (lock);
		}

		shared_ptr<asio::ip::tcp::socket> socket = _queue.front ();
		_queue.pop_front ();
		
		lock.unlock ();

		int frame = -1;

		struct timeval start;
		gettimeofday (&start, 0);
		
		try {
			frame = process (socket);
		} catch (std::exception& e) {
			cerr << "Error: " << e.what() << "\n";
		}
		
		socket.reset ();
		
		lock.lock ();

		if (frame >= 0) {
			struct timeval end;
			gettimeofday (&end, 0);
			stringstream s;
			s << "Encoded frame " << frame << " in " << (seconds (end) - seconds (start));
			_log->log (s.str ());
		}
		
		_worker_condition.notify_all ();
	}
}

void
Server::run (int num_threads)
{
	stringstream s;
	s << "Server starting with " << num_threads << " threads.";
	_log->log (s.str ());
	
	for (int i = 0; i < num_threads; ++i) {
		_worker_threads.push_back (new thread (bind (&Server::worker_thread, this)));
	}
	
	asio::ip::tcp::acceptor acceptor (_io_service, asio::ip::tcp::endpoint (asio::ip::tcp::v4(), Config::instance()->server_port ()));
	while (1) {
		shared_ptr<asio::ip::tcp::socket> socket (new asio::ip::tcp::socket (_io_service));
		acceptor.accept (*socket);

		mutex::scoped_lock lock (_worker_mutex);
		
		/* Wait until the queue has gone down a bit */
		while (int (_queue.size()) >= num_threads * 2) {
			_worker_condition.wait (lock);
		}
		
		_queue.push_back (socket);
		_worker_condition.notify_all ();
	}
}
