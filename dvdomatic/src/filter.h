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

class Filter
{
public:
	Filter (std::string const &, std::string const &, std::string const &, std::string const &);

	std::string id () const {
		return _id;
	}

	std::string name () const {
		return _name;
	}
	
	std::string pp () const {
		return _pp;
	}
	
	std::string vf () const {
		return _vf;
	}
	
	static std::vector<Filter const *> get_all ();
	static Filter const * get_from_id (std::string const &);
	static void setup_filters ();
	static std::pair<std::string, std::string> ffmpeg_strings (std::vector<Filter const *> const &);

private:

	std::string _id;
	std::string _name;
	std::string _vf;
	std::string _pp;

	static std::vector<Filter const *> _filters;
};
