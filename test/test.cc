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

#include <vector>
#include <list>
#include <libxml++/libxml++.h>
#include <libdcp/dcp.h>
#include "lib/config.h"
#include "lib/util.h"
#include "lib/ui_signaller.h"
#include "lib/film.h"
#include "lib/job_manager.h"
#include "lib/job.h"
#include "lib/cross.h"
#include "lib/server_finder.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE dcpomatic_test
#include <boost/test/unit_test.hpp>

using std::string;
using std::vector;
using std::min;
using std::cout;
using std::cerr;
using std::list;
using boost::shared_ptr;

class TestUISignaller : public UISignaller
{
public:
	/* No wakes in tests: we call ui_idle ourselves */
	void wake_ui ()
	{

	}
};

struct TestConfig
{
	TestConfig()
	{
		dcpomatic_setup();

		Config::instance()->set_num_local_encoding_threads (1);
		Config::instance()->set_server_port_base (61920);
		Config::instance()->set_default_dci_metadata (DCIMetadata ());
		Config::instance()->set_default_container (static_cast<Ratio*> (0));
		Config::instance()->set_default_dcp_content_type (static_cast<DCPContentType*> (0));

		ServerFinder::instance()->disable ();

		ui_signaller = new TestUISignaller ();
	}
};

BOOST_GLOBAL_FIXTURE (TestConfig);

boost::filesystem::path
test_film_dir (string name)
{
	boost::filesystem::path p;
	p /= "build";
	p /= "test";
	p /= name;
	return p;
}

shared_ptr<Film>
new_test_film (string name)
{
	boost::filesystem::path p = test_film_dir (name);
	if (boost::filesystem::exists (p)) {
		boost::filesystem::remove_all (p);
	}
	
	shared_ptr<Film> f = shared_ptr<Film> (new Film (p.string()));
	f->write_metadata ();
	return f;
}

void
check_file (boost::filesystem::path ref, boost::filesystem::path check)
{
	uintmax_t N = boost::filesystem::file_size (ref);
	BOOST_CHECK_EQUAL (N, boost::filesystem::file_size(check));
	FILE* ref_file = fopen_boost (ref, "rb");
	BOOST_CHECK (ref_file);
	FILE* check_file = fopen_boost (check, "rb");
	BOOST_CHECK (check_file);
	
	int const buffer_size = 65536;
	uint8_t* ref_buffer = new uint8_t[buffer_size];
	uint8_t* check_buffer = new uint8_t[buffer_size];

	while (N) {
		uintmax_t this_time = min (uintmax_t (buffer_size), N);
		size_t r = fread (ref_buffer, 1, this_time, ref_file);
		BOOST_CHECK_EQUAL (r, this_time);
		r = fread (check_buffer, 1, this_time, check_file);
		BOOST_CHECK_EQUAL (r, this_time);

		BOOST_CHECK_EQUAL (memcmp (ref_buffer, check_buffer, this_time), 0);
		N -= this_time;
	}

	delete[] ref_buffer;
	delete[] check_buffer;

	fclose (ref_file);
	fclose (check_file);
}

static void
note (libdcp::NoteType t, string n)
{
	if (t == libdcp::ERROR) {
		cerr << n << "\n";
	}
}

void
check_dcp (string ref, string check)
{
	libdcp::DCP ref_dcp (ref);
	ref_dcp.read ();
	libdcp::DCP check_dcp (check);
	check_dcp.read ();

	libdcp::EqualityOptions options;
	options.max_mean_pixel_error = 5;
	options.max_std_dev_pixel_error = 5;
	options.max_audio_sample_error = 255;
	options.cpl_names_can_differ = true;
	options.mxf_names_can_differ = true;
	
	BOOST_CHECK (ref_dcp.equals (check_dcp, options, boost::bind (note, _1, _2)));
}

void
check_xml (xmlpp::Element* ref, xmlpp::Element* test, list<string> ignore)
{
	BOOST_CHECK_EQUAL (ref->get_name (), test->get_name ());
	BOOST_CHECK_EQUAL (ref->get_namespace_prefix (), test->get_namespace_prefix ());

	if (find (ignore.begin(), ignore.end(), ref->get_name()) != ignore.end ()) {
		return;
	}
	    
	xmlpp::Element::NodeList ref_children = ref->get_children ();
	xmlpp::Element::NodeList test_children = test->get_children ();
	BOOST_CHECK_EQUAL (ref_children.size (), test_children.size ());

	xmlpp::Element::NodeList::iterator k = ref_children.begin ();
	xmlpp::Element::NodeList::iterator l = test_children.begin ();
	while (k != ref_children.end ()) {
		
		/* XXX: should be doing xmlpp::EntityReference, xmlpp::XIncludeEnd, xmlpp::XIncludeStart */

		xmlpp::Element* ref_el = dynamic_cast<xmlpp::Element*> (*k);
		xmlpp::Element* test_el = dynamic_cast<xmlpp::Element*> (*l);
		BOOST_CHECK ((ref_el && test_el) || (!ref_el && !test_el));
		if (ref_el && test_el) {
			check_xml (ref_el, test_el, ignore);
		}

		xmlpp::ContentNode* ref_cn = dynamic_cast<xmlpp::ContentNode*> (*k);
		xmlpp::ContentNode* test_cn = dynamic_cast<xmlpp::ContentNode*> (*l);
		BOOST_CHECK ((ref_cn && test_cn) || (!ref_cn && !test_cn));
		if (ref_cn && test_cn) {
			BOOST_CHECK_EQUAL (ref_cn->get_content(), test_cn->get_content ());
		}

		xmlpp::Attribute* ref_at = dynamic_cast<xmlpp::Attribute*> (*k);
		xmlpp::Attribute* test_at = dynamic_cast<xmlpp::Attribute*> (*l);
		BOOST_CHECK ((ref_at && test_at) || (!ref_at && !test_at));
		if (ref_at && test_at) {
			BOOST_CHECK_EQUAL (ref_at->get_name(), test_at->get_name ());
			BOOST_CHECK_EQUAL (ref_at->get_value(), test_at->get_value ());
		}

		++k;
		++l;
	}
}

void
check_xml (boost::filesystem::path ref, boost::filesystem::path test, list<string> ignore)
{
	xmlpp::DomParser* ref_parser = new xmlpp::DomParser (ref.string ());
	xmlpp::Element* ref_root = ref_parser->get_document()->get_root_node ();
	xmlpp::DomParser* test_parser = new xmlpp::DomParser (test.string ());
	xmlpp::Element* test_root = test_parser->get_document()->get_root_node ();

	check_xml (ref_root, test_root, ignore);
}

void
wait_for_jobs ()
{
	JobManager* jm = JobManager::instance ();
	while (jm->work_to_do ()) {
		ui_signaller->ui_idle ();
	}
	if (jm->errors ()) {
		for (list<shared_ptr<Job> >::iterator i = jm->_jobs.begin(); i != jm->_jobs.end(); ++i) {
			if ((*i)->finished_in_error ()) {
				cerr << (*i)->error_summary () << "\n"
				     << (*i)->error_details () << "\n";
			}
		}
	}
		
	BOOST_CHECK (!jm->errors());

	ui_signaller->ui_idle ();
}
