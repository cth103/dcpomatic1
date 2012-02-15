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
#include <cassert>
#include <iomanip>
#include <iostream>
#include "format.h"

using namespace std;

vector<Format *> Format::_formats;

Format::Format (int r, int dw, int dh, string const & n)
	: _ratio (r)
	, _dci_width (dw)
	, _dci_height (dh)
	, _nickname (n)
{

}

string
Format::name () const
{
	stringstream s;
	if (!_nickname.empty ()) {
		s << _nickname << " (";
	}

	s << setprecision(3) << (_ratio / 100.0) << ":1";

	if (!_nickname.empty ()) {
		s << ")";
	}

	return s.str ();
}

string
Format::get_as_metadata () const
{
	stringstream s;
	s << _ratio;
	return s.str ();
}

void
Format::setup_formats ()
{
	_formats.push_back (new Format (185, 1998, 1080, "Flat"));
	_formats.push_back (new Format (239, 2048, 858, "Scope"));
	_formats.push_back (new Format (137, 1480, 1080, "Academy"));
	_formats.push_back (new Format (185, 200, 100, "Test"));
}

Format *
Format::get_from_ratio (int r)
{
	vector<Format*>::iterator i = _formats.begin ();
	while (i != _formats.end() && (*i)->ratio_as_integer() != r) {
		++i;
	}

	if (i == _formats.end ()) {
		return 0;
	}

	return *i;
}

Format *
Format::get_from_nickname (string const & n)
{
	vector<Format*>::iterator i = _formats.begin ();
	while (i != _formats.end() && (*i)->nickname() != n) {
		++i;
	}

	if (i == _formats.end ()) {
		return 0;
	}

	return *i;
}

Format *
Format::get_from_metadata (string const & m)
{
	return get_from_ratio (atoi (m.c_str ()));
}

int
Format::get_as_index (Format const * f)
{
	vector<Format*>::size_type i = 0;
	while (i < _formats.size() && _formats[i] != f) {
		++i;
	}

	if (i == _formats.size ()) {
		return -1;
	}

	return i;
}

Format *
Format::get_from_index (int i)
{
	assert (i <= int(_formats.size ()));
	return _formats[i];
}

vector<Format*>
Format::get_all ()
{
	return _formats;
}
