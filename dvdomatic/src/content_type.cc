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

/** @file src/content_type.cc
 *  @brief A description of the type of content for a DCP (e.g. feature, trailer etc.)
 */

#include <cassert>
#include "content_type.h"

using namespace std;

vector<ContentType const *> ContentType::_content_types;

ContentType::ContentType (string p, string o, string d)
	: _pretty_name (p)
	, _opendcp_name (o)
	, _dcp_name (d)
{

}

void
ContentType::setup_content_types ()
{
	_content_types.push_back (new ContentType ("Feature", "feature", "FTR"));
	_content_types.push_back (new ContentType ("Short", "short", "SHR"));
	_content_types.push_back (new ContentType ("Trailer", "trailer", "TLR"));
	_content_types.push_back (new ContentType ("Test", "test", "TST"));
	_content_types.push_back (new ContentType ("Transitional", "transitional", "XSN"));
	_content_types.push_back (new ContentType ("Rating", "rating", "RTG"));
	_content_types.push_back (new ContentType ("Teaser", "teaster", "TSR"));
	_content_types.push_back (new ContentType ("Policy", "policy", "POL"));
	_content_types.push_back (new ContentType ("Public Service Announcement", "psa", "PSA"));
	_content_types.push_back (new ContentType ("Advertisement", "advertisement", "ADV"));
}

ContentType const *
ContentType::get_from_pretty_name (string n)
{
	for (vector<ContentType const *>::const_iterator i = _content_types.begin(); i != _content_types.end(); ++i) {
		if ((*i)->pretty_name() == n) {
			return *i;
		}
	}

	return 0;
}

ContentType const *
ContentType::get_from_index (int n)
{
	assert (n < int (_content_types.size ()));
	return _content_types[n];
}

int
ContentType::get_as_index (ContentType const * c)
{
	vector<ContentType*>::size_type i = 0;
	while (i < _content_types.size() && _content_types[i] != c) {
		++i;
	}

	if (i == _content_types.size ()) {
		return -1;
	}

	return i;
}

vector<ContentType const *>
ContentType::get_all ()
{
	return _content_types;
}
