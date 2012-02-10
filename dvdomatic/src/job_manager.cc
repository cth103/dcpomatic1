#include <iostream>
#include <boost/thread.hpp>
#include "job_manager.h"
#include "job.h"

using namespace std;

JobManager* JobManager::_instance = 0;

JobManager::JobManager ()
{
	boost::thread (boost::bind (&JobManager::scheduler, this));
}

void
JobManager::add (Job* j)
{
	boost::mutex::scoped_lock (_mutex);
	
	_jobs.push_back (j);
}

list<Job*>
JobManager::get () const
{
	boost::mutex::scoped_lock (_mutex);
	
	return _jobs;
}

void
JobManager::scheduler ()
{
	while (1) {
		{
			boost::mutex::scoped_lock (_mutex);
			int running = 0;
			Job* first_new = 0;
			for (list<Job*>::iterator i = _jobs.begin(); i != _jobs.end(); ++i) {
				if ((*i)->running ()) {
					++running;
				} else if (!(*i)->finished () && first_new == 0) {
					first_new = *i;
				}

				if (running == 0 && first_new) {
					first_new->start ();
				}
			}
		}

		sleep (1);
	}
}

JobManager *
JobManager::instance ()
{
	if (_instance == 0) {
		_instance = new JobManager ();
	}

	return _instance;
}
