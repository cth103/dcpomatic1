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
#include <unistd.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVOMATIC_PORT 6142
#define BACKLOG 8

using namespace std;

int main ()
{
	int fd = socket (AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		throw runtime_error ("could not open socket");
	}

	struct sockaddr_in server_address;
	memset (&server_address, 0, sizeof (server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons (SERVOMATIC_PORT);
	if (bind (fd, (struct sockaddr *) &server_address, sizeof (server_address)) < 0) {
		stringstream s;
		s << "could not bind (" << strerror (errno) << ")";
		throw runtime_error (s.str());
	}

	listen (fd, BACKLOG);

	while (1) {
		struct sockaddr_in client_address;
		socklen_t client_length = sizeof (client_address);
		int new_fd = accept (fd, (struct sockaddr *) &client_address, &client_length);
		if (new_fd < 0) {
			throw runtime_error ("error on accept");
		}
		
		char buffer[256];
		int n = read (new_fd, buffer, sizeof (buffer));
		if (n < 0) {
			throw runtime_error ("error reading from socket");
		}
		
		cout << buffer << "\n";
		
		n = write (new_fd, "Piss off", 9);
		if (n < 0) {
			throw runtime_error ("error writing to socket");
		}
		
		close (new_fd);
	}
	
	close (fd);

	return 0;
}
