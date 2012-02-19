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

#include <list>

class Server;

class Config
{
public:

	int num_local_encoding_threads () const {
		return _num_local_encoding_threads;
	}

	int server_port () const {
		return _server_port;
	}
	
	int colour_lut_index () const {
		return _colour_lut_index;
	}

	int j2k_bandwidth () const {
		return _j2k_bandwidth;
	}

	void set_num_local_encoding_threads (int n) {
		_num_local_encoding_threads = n;
	}

	void set_sever_port (int p) {
		_server_port = p;
	}
	
	void set_colour_lut_index (int i) {
		_colour_lut_index = i;
	}

	void set_j2k_bandwidth (int b) {
		_j2k_bandwidth = b;
	}

	std::list<Server*> servers () const;

	void write ();
	
	static Config* instance ();

private:
	Config ();
	std::string get_file () const;

	int _num_local_encoding_threads;
	int _server_port;
	int _colour_lut_index;
	int _j2k_bandwidth;

	std::list<Server*> _servers;

	static Config* _instance;

};
