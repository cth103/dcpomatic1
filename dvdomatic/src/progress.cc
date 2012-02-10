#include <iostream>
#include "progress.h"

using namespace std;

Progress::Progress ()
{
	descend (1);
}

void
Progress::set_progress (float p)
{
	boost::mutex::scoped_lock (_mutex);
	
	_stack.back().normalised = p;
}

/** @return fractional overall progress, or -1 if not known */
float
Progress::get_overall_progress () const
{
	boost::mutex::scoped_lock (_mutex);

	float overall = 0;
	float factor = 1;
	for (list<Level>::const_iterator i = _stack.begin(); i != _stack.end(); ++i) {
		factor *= i->allocation;
		overall += i->normalised * factor;
	}

	if (overall > 1) {
		overall = 1;
	}
	
	return overall;
}

void
Progress::ascend ()
{
	boost::mutex::scoped_lock (_mutex);
	
	assert (!_stack.empty ());
	float const a = _stack.back().allocation;
	_stack.pop_back ();
	_stack.back().normalised += a;
}

/** Descend down one level in terms of progress reporting; e.g. if
 *  there is a task which is split up into N subtasks, each of which
 *  report their progress from 0 to 100%, call descend() before executing
 *  each subtask, and ascend() afterwards to ensure that overall progress
 *  is reported correctly.
 *
 *  @param p Percentage (from 0 to 1) of the current task to allocate to the subtask.
 */
void
Progress::descend (float a)
{
	_stack.push_back (Level (a));
}
