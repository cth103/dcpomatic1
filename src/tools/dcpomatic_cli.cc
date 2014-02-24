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
#include <getopt.h>
#include <libdcp/version.h>
#include "lib/film.h"
#include "lib/filter.h"
#include "lib/transcode_job.h"
#include "lib/job_manager.h"
#include "lib/util.h"
#include "lib/scaler.h"
#include "lib/version.h"
#include "lib/cross.h"
#include "lib/config.h"
#include "lib/log.h"
#include "lib/ui_signaller.h"
#include "lib/server_finder.h"
#include "lib/json_server.h"

using std::string;
using std::cerr;
using std::cout;
using std::vector;
using std::pair;
using std::list;
using boost::shared_ptr;

static void
help (string n)
{
	cerr << "Syntax: " << n << " [OPTION] <FILM>\n"
	     << "  -v, --version      show DCP-o-matic version\n"
	     << "  -h, --help         show this help\n"
	     << "  -d, --deps         list DCP-o-matic dependency details and quit\n"
	     << "  -f, --flags        show flags passed to C++ compiler on build\n"
	     << "  -n, --no-progress  do not print progress to stdout\n"
	     << "  -r, --no-remote    do not use any remote servers\n"
	     << "  -j, --json <port>  run a JSON server on the specified port\n"
	     << "  -k, --keep-going   keep running even when the job is complete\n"
	     << "\n"
	     << "<FILM> is the film directory.\n";
}

int
main (int argc, char* argv[])
{
	string film_dir;
	bool progress = true;
	bool no_remote = false;
	int log_level = 0;
	int json_port = 0;
	bool keep_going = false;

	int option_index = 0;
	while (1) {
		static struct option long_options[] = {
			{ "version", no_argument, 0, 'v'},
			{ "help", no_argument, 0, 'h'},
			{ "deps", no_argument, 0, 'd'},
			{ "flags", no_argument, 0, 'f'},
			{ "no-progress", no_argument, 0, 'n'},
			{ "no-remote", no_argument, 0, 'r'},
			{ "log-level", required_argument, 0, 'l' },
			{ "json", required_argument, 0, 'j' },
			{ "keep-going", no_argument, 0, 'k' },
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long (argc, argv, "vhdfnrl:j:k", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'v':
			cout << "dcpomatic version " << dcpomatic_version << " " << dcpomatic_git_commit << "\n";
			exit (EXIT_SUCCESS);
		case 'h':
			help (argv[0]);
			exit (EXIT_SUCCESS);
		case 'd':
			cout << dependency_version_summary () << "\n";
			exit (EXIT_SUCCESS);
		case 'f':
			cout << dcpomatic_cxx_flags << "\n";
			exit (EXIT_SUCCESS);
		case 'n':
			progress = false;
			break;
		case 'r':
			no_remote = true;
			break;
		case 'l':
			log_level = atoi (optarg);
			break;
		case 'j':
			json_port = atoi (optarg);
			break;
		case 'k':
			keep_going = true;
			break;
		}
	}

	if (optind >= argc) {
		help (argv[0]);
		exit (EXIT_FAILURE);
	}

	film_dir = argv[optind];
			
	dcpomatic_setup ();
	ui_signaller = new UISignaller ();

	if (no_remote) {
		ServerFinder::instance()->disable ();
	}

	if (json_port) {
		new JSONServer (json_port);
	}

	cout << "DCP-o-matic " << dcpomatic_version << " git " << dcpomatic_git_commit;
	char buf[256];
	if (gethostname (buf, 256) == 0) {
		cout << " on " << buf;
	}
	cout << "\n";

	shared_ptr<Film> film;
	try {
		film.reset (new Film (film_dir));
		film->read_metadata ();
	} catch (std::exception& e) {
		cerr << argv[0] << ": error reading film `" << film_dir << "' (" << e.what() << ")\n";
		exit (EXIT_FAILURE);
	}

	film->log()->set_level ((Log::Level) log_level);

	cout << "\nMaking DCP for " << film->name() << "\n";
//	cout << "Content: " << film->content() << "\n";
//	pair<string, string> const f = Filter::ffmpeg_strings (film->filters ());
//	cout << "Filters: " << f.first << " " << f.second << "\n";

	film->make_dcp ();

	bool should_stop = false;
	bool first = true;
	bool error = false;
	while (!should_stop) {

		dcpomatic_sleep (5);

		list<shared_ptr<Job> > jobs = JobManager::instance()->get ();

		if (!first && progress) {
			cout << "\033[" << jobs.size() << "A";
			cout.flush ();
		}

		first = false;

		int unfinished = 0;
		int finished_in_error = 0;

		for (list<shared_ptr<Job> >::iterator i = jobs.begin(); i != jobs.end(); ++i) {
			if (progress) {
				cout << (*i)->name() << ": ";
				
				float const p = (*i)->progress ();
				
				if (p >= 0) {
					cout << (*i)->status() << "			    \n";
				} else {
					cout << ": Running	     \n";
				}
			}

			if (!(*i)->finished ()) {
				++unfinished;
			}

			if ((*i)->finished_in_error ()) {
				++finished_in_error;
				error = true;
			}

			if (!progress && (*i)->finished_in_error ()) {
				/* We won't see this error if we haven't been showing progress,
				   so show it now.
				*/
				cout << (*i)->status() << "\n";
			}
		}

		if (unfinished == 0 || finished_in_error != 0) {
			should_stop = true;
		}
	}

	if (keep_going) {
		while (1) {
			dcpomatic_sleep (3600);
		}
	}

	/* This is just to stop valgrind reporting leaks due to JobManager
	   indirectly holding onto codecs.
	*/
	JobManager::drop ();
	
	return error ? EXIT_FAILURE : EXIT_SUCCESS;
}

	  
