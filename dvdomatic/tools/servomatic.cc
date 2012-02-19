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
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include "config.h"
#include "image.h"
#include "exceptions.h"
#include "util.h"

#define BACKLOG 8
#define THREADS 4

using namespace std;
using namespace boost;

static vector<thread *> worker_threads;

struct Work {
	Work (shared_ptr<Image> i, int f)
		: image (i)
		, fd (f)
	{}
	
	shared_ptr<Image> image;
	int fd;
};

static std::list<Work> queue;
static mutex worker_mutex;
static condition worker_condition;

void
worker_thread ()
{
	while (1) {
		mutex::scoped_lock lock (worker_mutex);
		while (queue.empty ()) {
			worker_condition.wait (worker_mutex);
		}

		Work work = queue.front ();
		queue.pop_front ();
		
		lock.unlock ();
		cout << "Encoding " << work.image->frame() << "\n";
		work.image->encode_locally ();
		work.image->encoded()->send (work.fd);
		lock.lock ();

		worker_condition.notify_all ();
	}
}

int
main ()
{
	for (int i = 0; i < THREADS; ++i) {
		worker_threads.push_back (new thread (worker_thread));
	}
	
	int fd = socket (AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		throw NetworkError ("could not open socket");
	}

	int const o = 1;
	setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &o, sizeof (o));

	struct sockaddr_in server_address;
	memset (&server_address, 0, sizeof (server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons (Config::instance()->server_port ());
	if (::bind (fd, (struct sockaddr *) &server_address, sizeof (server_address)) < 0) {
		stringstream s;
		s << "could not bind to port " << Config::instance()->server_port() << " (" << strerror (errno) << ")";
		throw NetworkError (s.str());
	}

	listen (fd, BACKLOG);

	while (1) {
		struct sockaddr_in client_address;
		socklen_t client_length = sizeof (client_address);
		int new_fd = accept (fd, (struct sockaddr *) &client_address, &client_length);
		if (new_fd < 0) {
			throw NetworkError ("error on accept");
		}

		SocketReader reader (new_fd);
		
		char buffer[128];
		reader.read_indefinite ((uint8_t *) buffer, sizeof (buffer));
		reader.consume (strlen (buffer) + 1);

		string s (buffer);
		vector<string> b;
		split (b, s, is_any_of (" "));

		if (b.size() == 5 && b[0] == "encode") {
			int const w = atoi (b[2].c_str ());
			int const h = atoi (b[3].c_str ());
			shared_ptr<Image> image (new Image (atoi (b[1].c_str ()), w, h, atoi (b[4].c_str ())));
			for (int i = 0; i < 3; ++i) {
				reader.read_definite_and_consume ((uint8_t *) image->component_buffer (i), w * h * sizeof (int));
			}
			
			{
				mutex::scoped_lock lock (worker_mutex);
				
				/* Wait until the queue has gone down a bit */
				while (int (queue.size()) >= THREADS * 2) {
					worker_condition.wait (lock);
				}

				cout << "Queue " << image->frame() << "\n";
				queue.push_back (Work (image, new_fd));
				worker_condition.notify_all ();
			}

			fd_write (new_fd, (uint8_t *) "OK", 3);
			
		} else {
			close (new_fd);
		}
	}
	
	close (fd);

	return 0;
}
