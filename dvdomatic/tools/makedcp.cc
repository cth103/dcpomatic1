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

#include <iostream>
#include <iomanip>
#include <boost/program_options.hpp>
#include "format.h"
#include "film.h"
#include "filter.h"
#include "transcode_job.h"
#include "make_mxf_job.h"
#include "make_dcp_job.h"
#include "job_manager.h"
#include "ab_transcode_job.h"

using namespace std;

int main (int argc, char* argv[])
{
	string film_dir;
	
	boost::program_options::options_description desc ("Allowed options");
	desc.add_options ()
		("help", "give help")
		("film", boost::program_options::value<string> (&film_dir), "film")
		;
	boost::program_options::positional_options_description pos;
	pos.add ("film", 1);

	boost::program_options::variables_map vm;

	/* Ha ha ha! */
	boost::program_options::parsed_options parsed = boost::program_options::command_line_parser (argc, argv).
		options(desc).allow_unregistered().positional(pos).run();
	
	boost::program_options::store (parsed, vm);
	boost::program_options::notify (vm);
		
	Format::setup_formats ();
	Filter::setup_filters ();
	ContentType::setup_content_types ();

	Film* film = 0;
	try {
		film = new Film (film_dir, true);
	} catch (exception& e) {
		cerr << argv[0] << ": error reading film `" << film_dir << "' (" << e.what() << ")\n";
		exit (EXIT_FAILURE);
	}

	cout << "\nMaking ";
	if (film->dcp_ab ()) {
		cout << "A/B ";
	}
	cout << "DCP for " << film->name() << "\n";
	cout << "Content: " << film->content() << "\n";
	pair<string, string> const f = Filter::ffmpeg_strings (film->filters ());
	cout << "Filters: " << f.first << " " << f.second << "\n";

	film->make_dcp ();

	list<Job*> jobs = JobManager::instance()->get ();

	bool all_done = false;
	bool first = true;
	while (!all_done) {
		
		sleep (5);
		
		if (!first) {
			cout << "\033[" << jobs.size() << "A";
			cout.flush ();
		}

		first = false;
		
		all_done = true;
		for (list<Job*>::iterator i = jobs.begin(); i != jobs.end(); ++i) {
			cout << (*i)->name() << ": " << fixed << setprecision(1) << ((*i)->get_overall_progress() * 100) << "%             \n";
			if (!(*i)->finished ()) {
				all_done = false;
			}
		}
	}

	return 0;
}

	  
