/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

#include <boost/filesystem.hpp>

class Film;

extern boost::filesystem::path private_data;

extern void wait_for_jobs ();
extern boost::shared_ptr<Film> new_test_film (std::string);
extern void check_dcp (std::string, std::string);
extern void check_xml (boost::filesystem::path, boost::filesystem::path, std::list<std::string>);
extern void check_file (boost::filesystem::path, boost::filesystem::path);
extern boost::filesystem::path test_film_dir (std::string);
