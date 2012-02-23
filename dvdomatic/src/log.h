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
#include <boost/thread/mutex.hpp>

class Log
{
public:
	Log (std::string);

	enum Level {
		STANDARD = 0,
		VERBOSE = 1
	};

	void log (std::string, Level level = STANDARD);

	void set_level (Level);

private:
	boost::mutex _mutex;
	std::string _file;
	Level _level;
};
