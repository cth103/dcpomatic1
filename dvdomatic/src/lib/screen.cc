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

#include <boost/algorithm/string.hpp>
#include "screen.h"
#include "format.h"
#include "exceptions.h"

using namespace std;
using namespace boost;

Screen::Screen (string n)
	: _name (n)
{

}

void
Screen::add_geometry (Format const * f, Position p, Size s)
{
	_geometries.insert (make_pair (f, Geometry (p, s)));
}

Position
Screen::position (Format const * f) const
{
	GeometryMap::const_iterator i = _geometries.find (f);
	if (i == _geometries.end ()) {
		throw PlayError ("format not found for screen");
	}

	return i->second.position;
}

Size
Screen::size (Format const * f) const
{
	GeometryMap::const_iterator i = _geometries.find (f);
	if (i == _geometries.end ()) {
		throw PlayError ("format not found for screen");
	}

	return i->second.size;
}

string
Screen::get_as_metadata () const
{
	stringstream s;
	s << _name;

	for (GeometryMap::const_iterator i = _geometries.begin(); i != _geometries.end(); ++i) {
		s << " " << i->first->nickname()
		  << " " << i->second.position.x << " " << i->second.position.y
		  << " " << i->second.size.width << " " << i->second.size.height;
	}

	return s.str ();
}

Screen *
Screen::create_from_metadata (string v)
{
	vector<string> b;
	split (b, v, is_any_of (" "));

	if (b.size() < 1) {
		return 0;
	}

	Screen* s = new Screen (b[0]);

	vector<string>::size_type i = 1;
	while (b.size() > i) {
		if (b.size() >= (i + 5)) {
			s->add_geometry (
				Format::get_from_nickname (b[i].c_str()),
				Position (atoi (b[i+1].c_str()), atoi (b[i+2].c_str())),
				Size (atoi (b[i+3].c_str()), atoi (b[i+4].c_str()))
				);
		}
		i += 5;
	}

	return s;
}
