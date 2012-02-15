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

#include <boost/filesystem.hpp>
#include "format.h"
#include "film.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE dvdomatic_test
#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_AUTO_TEST_CASE (film_metadata_test)
{
	string const test_film = "build/test/film";
	
	if (boost::filesystem::exists (test_film)) {
		boost::filesystem::remove_all (test_film);
	}

	BOOST_CHECK_THROW (new Film ("build/test/film", true), runtime_error);
	
	Film f (test_film, false);
	BOOST_CHECK (f.format() == 0);
	BOOST_CHECK (f.dcp_content_type() == 0);
	BOOST_CHECK (f.filters ().empty());
}

BOOST_AUTO_TEST_CASE (format_test)
{
	Format::setup_formats ();
	
	Format* f = Format::get_from_nickname ("Flat");
	BOOST_CHECK (f);
	BOOST_CHECK_EQUAL (f->ratio_as_integer(), 185);
	
	f = Format::get_from_nickname ("Scope");
	BOOST_CHECK (f);
	BOOST_CHECK_EQUAL (f->ratio_as_integer(), 239);
}
