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

#include <iostream>
#include <iomanip>
#include <exception>
#include <boost/program_options.hpp>
#include "format.h"
#include "film.h"
#include "filter.h"
#include "util.h"
#include "scaler.h"
#include "server.h"
#include "dcp_video_frame.h"
#include "options.h"
#include "decoder.h"
#include "exceptions.h"
#include "scaler.h"
#include "log.h"
#include "decoder_factory.h"

using namespace std;
using namespace boost;

static Server* server;
static Log log_ ("servomatictest.log");

void
process_video (shared_ptr<Image> image, int frame)
{
	shared_ptr<DCPVideoFrame> local (new DCPVideoFrame (image, Size (1024, 1024), Scaler::get_from_id ("bicubic"), frame, 24, "", 0, 250000000, &log_));
	shared_ptr<DCPVideoFrame> remote (new DCPVideoFrame (image, Size (1024, 1024), Scaler::get_from_id ("bicubic"), frame, 24, "", 0, 250000000, &log_));

#if defined(DEBUG_HASH)
	cout << "Frame " << frame << ":\n";
#else
	cout << "Frame " << frame << ": ";
	cout.flush ();
#endif	

	shared_ptr<EncodedData> local_encoded = local->encode_locally ();
	shared_ptr<EncodedData> remote_encoded;

	string remote_error;
	try {
		remote_encoded = remote->encode_remotely (server);
	} catch (NetworkError& e) {
		remote_error = e.what ();
	}

#if defined(DEBUG_HASH)
	cout << "Frame " << frame << ": ";
	cout.flush ();
#endif	

	if (!remote_error.empty ()) {
		cout << "\033[0;31mnetwork problem: " << remote_error << "\033[0m\n";
		return;
	}

	if (local_encoded->size() != remote_encoded->size()) {
		cout << "\033[0;31msizes differ\033[0m\n";
		return;
	}
		
	uint8_t* p = local_encoded->data();
	uint8_t* q = remote_encoded->data();
	for (int i = 0; i < local_encoded->size(); ++i) {
		if (*p++ != *q++) {
			cout << "\033[0;31mdata differ\033[0m at byte " << i << "\n";
			return;
		}
	}

	cout << "\033[0;32mgood\033[0m\n";
}

int
main (int argc, char* argv[])
{
	string film_dir;
	string server_host;
	
	boost::program_options::options_description desc ("Allowed options");
	desc.add_options ()
		("help", "give help")
		("server", boost::program_options::value<string> (&server_host)->required(), "server hostname")
		("film", boost::program_options::value<string> (&film_dir)->required(), "film")
		;

	boost::program_options::variables_map vm;
	boost::program_options::store (boost::program_options::parse_command_line (argc, argv, desc), vm);

	if (vm.count ("help")) {
		cout << desc << "\n";
		return 0;
	}
	
        boost::program_options::notify (vm);

	dvdomatic_setup ();

	server = new Server (server_host, 1);
	Film film (film_dir, true);

	shared_ptr<Options> opt (new Options ("fred", "jim", "sheila"));
	opt->out_size = Size (1024, 1024);
	opt->apply_crop = false;
	opt->decode_audio = false;

	Decoder* decoder = decoder_factory (film.state_copy(), opt, 0, &log_);
	try {
		decoder->Video.connect (sigc::ptr_fun (process_video));
		decoder->go ();
	} catch (std::exception& e) {
		cerr << "Error: " << e.what() << "\n";
	}

	delete decoder;

	return 0;
}
