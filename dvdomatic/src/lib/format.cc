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

/** @file src/format.cc
 *  @brief Class to describe a format (aspect ratio) that a Film should
 *  be shown in.
 */

#include <sstream>
#include <cstdlib>
#include <cassert>
#include <iomanip>
#include <iostream>
#include "format.h"

using namespace std;

vector<Format const *> Format::_formats;

/** @param r Ratio multiplied by 100 (e.g. 185)
 *  @param dci Size (in pixels) of the DCI specification (e.g. 1998x1080)
 *  @param n Nick name (e.g. Flat)
 *  @param d Text to use as part of a DCP name (e.g. F)
 */
Format::Format (int r, Size dci, string n, string d)
	: _ratio (r)
	, _dci_size (dci)
	, _nickname (n)
	, _dcp_name (d)
{

}

/** @return A name to be presented to the user */
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

/** @return Identifier for this format as metadata for a Film's metadata file */
string
Format::as_metadata () const
{
	stringstream s;
	s << _ratio;
	return s.str ();
}

/** Fill our _formats vector with all available formats */
void
Format::setup_formats ()
{
	_formats.push_back (new Format (185, Size (1998, 1080), "Flat", "F"));
	_formats.push_back (new Format (239, Size (2048, 858), "Scope", "S"));
	_formats.push_back (new Format (137, Size (1480, 1080), "Academy", "133"));
}

/** @param r Ratio multiplied by 100.
 *  @return Matching Format, or 0.
 */
Format const *
Format::from_ratio (int r)
{
	vector<Format const *>::iterator i = _formats.begin ();
	while (i != _formats.end() && (*i)->ratio_as_integer() != r) {
		++i;
	}

	if (i == _formats.end ()) {
		return 0;
	}

	return *i;
}

/** @param n Nickname.
 *  @return Matching Format, or 0.
 */
Format const *
Format::from_nickname (string n)
{
	vector<Format const *>::iterator i = _formats.begin ();
	while (i != _formats.end() && (*i)->nickname() != n) {
		++i;
	}

	if (i == _formats.end ()) {
		return 0;
	}

	return *i;
}

/** @param m Metadata, as returned from as_metadata().
 *  @return Matching Format, or 0.
 */
Format const *
Format::from_metadata (string m)
{
	return from_ratio (atoi (m.c_str ()));
}

/** @param f A Format.
 *  @return Index of f within our static list, or -1.
 */
int
Format::as_index (Format const * f)
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

/** @param i An index returned from as_index().
 *  @return Corresponding Format.
 */
Format const *
Format::from_index (int i)
{
	assert (i >= 0 && i < int(_formats.size ()));
	return _formats[i];
}

/** @return All available formats */
vector<Format const *>
Format::all ()
{
	return _formats;
}
