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
