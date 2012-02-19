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

#include <string>
#include <vector>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "server.h"

using namespace std;
using namespace boost;

Server *
Server::create_from_metadata (string v)
{
	vector<string> b;
	split (b, v, is_any_of (" "));

	if (b.size() != 2) {
		return 0;
	}

	return new Server (b[0], atoi (b[1].c_str ()));
}

string
Server::get_as_metadata () const
{
	stringstream s;
	s << _host_name << " " << _threads;
	return s.str ();
}
