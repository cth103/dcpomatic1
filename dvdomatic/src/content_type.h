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

class ContentType
{
public:
	ContentType (std::string, std::string);

	std::string pretty_name () const {
		return _pretty_name;
	}
	
	std::string opendcp_name () const {
		return _opendcp_name;
	}

	static ContentType const * get_from_pretty_name (std::string);
	static ContentType const * get_from_index (int);
	static int get_as_index (ContentType const *);
	static std::vector<ContentType const *> get_all ();
	static void setup_content_types ();

private:
	std::string _pretty_name;
	std::string _opendcp_name;

	static std::vector<ContentType const *> _content_types;
};
     
