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
	
	Film film (argv[1]);

	JobManager::instance()->add (new TranscodeJob (&film));
	JobManager::instance()->add (new MakeMXFJob (&film, MakeMXFJob::VIDEO));
	JobManager::instance()->add (new MakeMXFJob (&film, MakeMXFJob::AUDIO));
	JobManager::instance()->add (new MakeDCPJob (&film));

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

	  
