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

#include <sstream>
#include <cstdlib>
#include <fstream>
#include "config.h"
#include "server.h"
#include "scaler.h"
#include "screen.h"
#include "filter.h"

using namespace std;
using namespace boost;

Config* Config::_instance = 0;

/** Construct default configuration */
Config::Config ()
	: _num_local_encoding_threads (2)
	, _server_port (6192)
	, _colour_lut_index (0)
	, _j2k_bandwidth (250000000)
	, _reference_scaler (Scaler::from_id ("bicubic"))
{
	ifstream f (file().c_str ());
	string line;
	while (getline (f, line)) {
		if (line.empty ()) {
			continue;
		}

		if (line[0] == '#') {
			continue;
		}

		size_t const s = line.find (' ');
		if (s == string::npos) {
			continue;
		}
		
		string const k = line.substr (0, s);
		string const v = line.substr (s + 1);

		if (k == "num_local_encoding_threads") {
			_num_local_encoding_threads = atoi (v.c_str ());
		} else if (k == "server_port") {
			_server_port = atoi (v.c_str ());
		} else if (k == "colour_lut_index") {
			_colour_lut_index = atoi (v.c_str ());
		} else if (k == "j2k_bandwidth") {
			_j2k_bandwidth = atoi (v.c_str ());
		} else if (k == "reference_scaler") {
			_reference_scaler = Scaler::from_id (v);
		} else if (k == "reference_filter") {
			_reference_filters.push_back (Filter::from_id (v));
		} else if (k == "server") {
			_servers.push_back (Server::create_from_metadata (v));
		} else if (k == "screen") {
			_screens.push_back (Screen::create_from_metadata (v));
		}
	}

	Changed ();
}

/** @return Filename to write configuration to */
string
Config::file () const
{
	stringstream s;
	s << getenv ("HOME") << "/.dvdomatic";
	return s.str ();
}

/** @return Singleton instance */
Config *
Config::instance ()
{
	if (_instance == 0) {
		_instance = new Config;
	}

	return _instance;
}

/** Write our configuration to disk */
void
Config::write () const
{
	ofstream f (file().c_str ());
	f << "num_local_encoding_threads " << _num_local_encoding_threads << "\n"
	  << "server_port " << _server_port << "\n"
	  << "colour_lut_index " << _colour_lut_index << "\n"
	  << "j2k_bandwidth " << _j2k_bandwidth << "\n"
	  << "reference_scaler " << _reference_scaler->id () << "\n";

	for (vector<Filter const *>::const_iterator i = _reference_filters.begin(); i != _reference_filters.end(); ++i) {
		f << "reference_filter " << (*i)->id () << "\n";
	}
	
	for (vector<Server*>::const_iterator i = _servers.begin(); i != _servers.end(); ++i) {
		f << "server " << (*i)->as_metadata () << "\n";
	}

	for (vector<shared_ptr<Screen> >::const_iterator i = _screens.begin(); i != _screens.end(); ++i) {
		f << "screen " << (*i)->as_metadata () << "\n";
	}
}
