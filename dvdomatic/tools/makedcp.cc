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
#include "format.h"
#include "film.h"
#include "filter.h"
#include "transcode_job.h"
#include "make_mxf_job.h"
#include "make_dcp_job.h"
#include "job_manager.h"

using namespace std;

int main (int argc, char* argv[])
{
	if (argc != 2) {
		cerr << "Syntax: " << argv[0] << " <film>\n";
		exit (EXIT_FAILURE);
	}

	Format::setup_formats ();
	Filter::setup_filters ();
	ContentType::setup_content_types ();

	Film* film = 0;
	try {
		film = new Film (argv[1], true);
	} catch (runtime_error& e) {
		cerr << argv[0] << ": error reading film (" << e.what() << ")\n";
		exit (EXIT_FAILURE);
	}

	JobManager::instance()->add (new TranscodeJob (film));
	JobManager::instance()->add (new MakeMXFJob (film, MakeMXFJob::VIDEO));
	JobManager::instance()->add (new MakeMXFJob (film, MakeMXFJob::AUDIO));
	JobManager::instance()->add (new MakeDCPJob (film));

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

	  
