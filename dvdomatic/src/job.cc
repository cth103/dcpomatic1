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

#include <boost/thread.hpp>
#include "job.h"

using namespace std;

Job::Job (Film* f)
	: _film (f)
	, _start_time (0)
{

}

/** Start the job in a separate thread, returning immediately */
void
Job::start ()
{
	set_state (RUNNING);
	_start_time = time (0);
	boost::thread (boost::bind (&Job::run, this));
}

bool
Job::running () const
{
	boost::mutex::scoped_lock (_state_mutex);
	return _state == RUNNING;
}

bool
Job::finished () const
{
	boost::mutex::scoped_lock (_state_mutex);
	return _state == FINISHED_OK || _state == FINISHED_ERROR;
}

bool
Job::finished_ok () const
{
	boost::mutex::scoped_lock (_state_mutex);
	return _state == FINISHED_OK;
}

bool
Job::finished_in_error () const
{
	boost::mutex::scoped_lock (_state_mutex);
	return _state == FINISHED_ERROR;
}

void
Job::set_state (State s)
{
	boost::mutex::scoped_lock (_state_mutex);
	_state = s;
}

float
Job::progress () const
{
	return _progress.get_overall_progress ();
}

/** A hack to work around our lack of cross-thread
 *  signalling; this emits Finished, and listeners
 *  assume that it will be emitted in the GUI thread,
 *  so this method must be called from the GUI thread.
 */
void
Job::emit_finished ()
{
	Finished ();
}

int
Job::elapsed_time () const
{
	if (_start_time == 0) {
		return 0;
	}
	
	return time (0) - _start_time;
}
