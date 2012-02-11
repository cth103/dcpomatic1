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

#include <cassert>
#include "content_type.h"

using namespace std;

vector<ContentType *> ContentType::_content_types;

ContentType::ContentType (string const & p, string const & o)
	: _pretty_name (p)
	, _opendcp_name (o)
{

}

void
ContentType::setup_content_types ()
{
	_content_types.push_back (new ContentType ("Feature", "feature"));
	_content_types.push_back (new ContentType ("Short", "short"));
	_content_types.push_back (new ContentType ("Trailer", "trailer"));
	_content_types.push_back (new ContentType ("Test", "test"));
	_content_types.push_back (new ContentType ("Transitional", "transitional"));
	_content_types.push_back (new ContentType ("Rating", "rating"));
	_content_types.push_back (new ContentType ("Teaser", "teaster"));
	_content_types.push_back (new ContentType ("Policy", "policy"));
	_content_types.push_back (new ContentType ("Public Service Announcement", "psa"));
	_content_types.push_back (new ContentType ("Advertisement", "advertisement"));
}

ContentType *
ContentType::get_from_pretty_name (string const & n)
{
	for (vector<ContentType*>::const_iterator i = _content_types.begin(); i != _content_types.end(); ++i) {
		if ((*i)->pretty_name() == n) {
			return *i;
		}
	}

	return 0;
}

ContentType *
ContentType::get_from_index (int n)
{
	assert (n < int (_content_types.size ()));
	return _content_types[n];
}

int
ContentType::get_as_index (ContentType* c)
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

vector<ContentType*>
ContentType::get_all ()
{
	return _content_types;
}
