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

#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "format.h"
#include "film.h"
#include "filter.h"
#include "job_manager.h"
#include "util.h"
#include "exceptions.h"
#include "dvd.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE dvdomatic_test
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace boost;

BOOST_AUTO_TEST_CASE (film_metadata_test)
{
	dvdomatic_setup ();
	
	string const test_film = "build/test/film";
	
	if (boost::filesystem::exists (test_film)) {
		boost::filesystem::remove_all (test_film);
	}

	BOOST_CHECK_THROW (new Film ("build/test/film", true), OpenFileError);
	
	Film f (test_film, false);
	BOOST_CHECK (f.format() == 0);
	BOOST_CHECK (f.dcp_content_type() == 0);
	BOOST_CHECK (f.filters ().empty());

	f.set_name ("fred");
	BOOST_CHECK_THROW (f.set_content ("jim"), OpenFileError);
	f.set_dcp_long_name ("sheila");
	f.set_dcp_content_type (ContentType::from_pretty_name ("Short"));
	f.set_format (Format::from_nickname ("Flat"));
	f.set_left_crop (1);
	f.set_right_crop (2);
	f.set_top_crop (3);
	f.set_bottom_crop (4);
	vector<Filter const *> f_filters;
	f_filters.push_back (Filter::from_id ("pphb"));
	f_filters.push_back (Filter::from_id ("unsharp"));
	f.set_filters (f_filters);
	f.set_dcp_frames (42);
	f.set_dcp_ab (true);
	f.write_metadata ();

	stringstream s;
	s << "diff -u test/metadata.ref " << test_film << "/metadata";
	BOOST_CHECK_EQUAL (::system (s.str().c_str ()), 0);

	Film g (test_film, true);

	BOOST_CHECK_EQUAL (g.name(), "fred");
	BOOST_CHECK_EQUAL (g.dcp_long_name(), "sheila");
	BOOST_CHECK_EQUAL (g.dcp_content_type(), ContentType::from_pretty_name ("Short"));
	BOOST_CHECK_EQUAL (g.format(), Format::from_nickname ("Flat"));
	BOOST_CHECK_EQUAL (g.left_crop(), 1);
	BOOST_CHECK_EQUAL (g.right_crop(), 2);
	BOOST_CHECK_EQUAL (g.top_crop(), 3);
	BOOST_CHECK_EQUAL (g.bottom_crop(), 4);
	vector<Filter const *> g_filters = g.filters ();
	BOOST_CHECK_EQUAL (g_filters.size(), 2);
	BOOST_CHECK_EQUAL (g_filters.front(), Filter::from_id ("pphb"));
	BOOST_CHECK_EQUAL (g_filters.back(), Filter::from_id ("unsharp"));
	BOOST_CHECK_EQUAL (g.dcp_frames(), 42);
	BOOST_CHECK_EQUAL (g.dcp_ab(), true);
	
	g.write_metadata ();
	BOOST_CHECK_EQUAL (::system (s.str().c_str ()), 0);
}

BOOST_AUTO_TEST_CASE (format_test)
{
	Format::setup_formats ();
	
	Format const * f = Format::from_nickname ("Flat");
	BOOST_CHECK (f);
	BOOST_CHECK_EQUAL (f->ratio_as_integer(), 185);
	
	f = Format::from_nickname ("Scope");
	BOOST_CHECK (f);
	BOOST_CHECK_EQUAL (f->ratio_as_integer(), 239);
}

BOOST_AUTO_TEST_CASE (util_test)
{
	string t = "Hello this is a string \"with quotes\" and indeed without them";
	vector<string> b = split_at_spaces_considering_quotes (t);
	vector<string>::iterator i = b.begin ();
	BOOST_CHECK_EQUAL (*i++, "Hello");
	BOOST_CHECK_EQUAL (*i++, "this");
	BOOST_CHECK_EQUAL (*i++, "is");
	BOOST_CHECK_EQUAL (*i++, "a");
	BOOST_CHECK_EQUAL (*i++, "string");
	BOOST_CHECK_EQUAL (*i++, "with quotes");
	BOOST_CHECK_EQUAL (*i++, "and");
	BOOST_CHECK_EQUAL (*i++, "indeed");
	BOOST_CHECK_EQUAL (*i++, "without");
	BOOST_CHECK_EQUAL (*i++, "them");
}

BOOST_AUTO_TEST_CASE (dvd_test)
{
	vector<uint64_t> const t = dvd_titles ("test/dvd");
	BOOST_CHECK_EQUAL (t.size(), 4);
	BOOST_CHECK_EQUAL (t[1], 0);
	BOOST_CHECK_EQUAL (t[2], 14);
	BOOST_CHECK_EQUAL (t[3], 7);
}
