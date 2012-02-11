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
					break;
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
