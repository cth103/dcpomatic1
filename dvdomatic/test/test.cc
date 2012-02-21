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
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE dvdomatic_test
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace boost;

BOOST_AUTO_TEST_CASE (film_metadata_test)
{
	Filter::setup_filters ();
	Format::setup_formats ();
	ContentType::setup_content_types ();
	Scaler::setup_scalers ();
	
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
	f.set_dcp_content_type (ContentType::get_from_pretty_name ("Short"));
	f.set_format (Format::get_from_nickname ("Flat"));
	f.set_left_crop (1);
	f.set_right_crop (2);
	f.set_top_crop (3);
	f.set_bottom_crop (4);
	vector<Filter const *> f_filters;
	f_filters.push_back (Filter::get_from_id ("pphb"));
	f_filters.push_back (Filter::get_from_id ("unsharp"));
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
	BOOST_CHECK_EQUAL (g.dcp_content_type(), ContentType::get_from_pretty_name ("Short"));
	BOOST_CHECK_EQUAL (g.format(), Format::get_from_nickname ("Flat"));
	BOOST_CHECK_EQUAL (g.left_crop(), 1);
	BOOST_CHECK_EQUAL (g.right_crop(), 2);
	BOOST_CHECK_EQUAL (g.top_crop(), 3);
	BOOST_CHECK_EQUAL (g.bottom_crop(), 4);
	vector<Filter const *> g_filters = g.filters ();
	BOOST_CHECK_EQUAL (g_filters.size(), 2);
	BOOST_CHECK_EQUAL (g_filters.front(), Filter::get_from_id ("pphb"));
	BOOST_CHECK_EQUAL (g_filters.back(), Filter::get_from_id ("unsharp"));
	BOOST_CHECK_EQUAL (g.dcp_frames(), 42);
	BOOST_CHECK_EQUAL (g.dcp_ab(), true);
	
	g.write_metadata ();
	BOOST_CHECK_EQUAL (::system (s.str().c_str ()), 0);
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

bool
compare (string ref, string test, list<string> exclude)
{
	ifstream r (ref.c_str ());
	ifstream t (test.c_str ());

	while (r.good ()) {
		string rl;
		getline (r, rl);
		string tl;
		getline (t, tl);

		bool ex = false;
		for (list<string>::iterator i = exclude.begin(); i != exclude.end(); ++i) {
			if (rl.find (*i) != string::npos && tl.find (*i) != string::npos) {
				ex = true;
			}
		}

		if (!ex && rl != tl) {
			cerr << "Fail:\n" << rl << "\n" << tl << "\n";
			return true;
		}
	}

	return false;
}

BOOST_AUTO_TEST_CASE (make_dcp_test)
{
	string const dcp_name = "FOO-BAR-BAZ";
	
	string const ref_film = "test/film";
	string const ref_dcp = ref_film + "/" + dcp_name;
	string const ref_pkl = ref_dcp + "/bdb4ae0a-0d09-4554-8557-0b4260f4c359_pkl.xml";
	string const ref_cpl = ref_dcp + "/08dd6e45-83b5-41dc-9179-d7c59f597a12_cpl.xml";
	string const test_film = "build/test/film";
	string const test_dcp = test_film + "/" + dcp_name;
	
	if (boost::filesystem::exists (test_film)) {
		boost::filesystem::remove_all (test_film);
	}

	Film f (test_film, false);
	f.write_metadata ();
	boost::filesystem::copy_file ("test/zombie.mpeg", "build/test/film/zombie.mpeg");
	f.set_content ("zombie.mpeg");
	f.set_dcp_frames (5);
	f.set_dcp_content_type (ContentType::get_from_pretty_name ("Test"));
	f.set_dcp_long_name (dcp_name);

	BOOST_CHECK_EQUAL (f.audio_channels(), 2);
	BOOST_CHECK_EQUAL (f.audio_sample_rate(), 48000);
	BOOST_CHECK_EQUAL (audio_sample_format_to_string (f.audio_sample_format()), "S16");
	
	f.set_format (Format::get_from_nickname ("Flat"));
	f.make_dcp ();

	while (JobManager::instance()->work_to_do ()) {
		sleep (1);
	}

	{
		stringstream s;
		s << "diff -ur test/film/j2c " << test_film << "/j2c";
		BOOST_CHECK_EQUAL (::system (s.str().c_str ()), 0);
	}

	{
		stringstream s;
		s << "diff -ur test/film/wavs " << test_film << "/wavs";
		BOOST_CHECK_EQUAL (::system (s.str().c_str ()), 0);
	}

	{
		stringstream s;
		s << "diff -u test/film/metadata " << test_film << "/metadata";
		BOOST_CHECK_EQUAL (::system (s.str().c_str ()), 0);
	}

	/* Find the test pkl and cpl */
	string test_pkl;
	string test_cpl;

	for (filesystem::directory_iterator i = filesystem::directory_iterator (test_dcp); i != filesystem::directory_iterator(); ++i) {
#if BOOST_FILESYSTEM_VERSION == 3		
		string const t = filesystem::path(*i).generic_string ();
#else
		string const t = i->string ();
#endif
		if (algorithm::ends_with (t, "cpl.xml")) {
			test_cpl = t;
		} else if (algorithm::ends_with (t, "pkl.xml")) {
			test_pkl = t;
		}
	}

	{
		list<string> exclude;
		exclude.push_back ("urn:uuid");
		exclude.push_back ("urn:uri");
		exclude.push_back ("<IssueDate>");
		exclude.push_back ("<LabelText>");
		exclude.push_back ("<Hash>");
		BOOST_CHECK_EQUAL (compare (ref_cpl, test_cpl, exclude), false);
	}

	{
		list<string> exclude;
		exclude.push_back ("urn:uuid");
		exclude.push_back ("<IssueDate>");
		exclude.push_back ("<Hash>");
		BOOST_CHECK_EQUAL (compare (ref_pkl, test_pkl, exclude), false);
	}
}
