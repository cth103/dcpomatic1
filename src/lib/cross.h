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
#include <boost/filesystem.hpp>

class Log;

#ifdef DVDOMATIC_WINDOWS
#define WEXITSTATUS(w) (w)
#endif

extern void dvdomatic_sleep (int);
extern std::string cpu_info ();
extern void run_ffprobe (boost::filesystem::path, boost::filesystem::path, boost::shared_ptr<Log>);
extern std::list<std::pair<std::string, std::string> > mount_info ();
