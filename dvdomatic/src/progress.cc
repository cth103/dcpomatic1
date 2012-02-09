#include <glib/gatomic.h>
#include "progress.h"

Progress::Progress ()
	: _progress (0)
	, _total (0)
{

}

void
Progress::set_progress (int p)
{
	g_atomic_int_set (&_progress, p);
}

void
Progress::set_total (int t)
{
	g_atomic_int_set (&_total, t);
}

void
Progress::set_done (bool d)
{
	g_atomic_int_set (&_done, (d ? 1 : 0));
}

/** @return fractional progress, or -1 if not known */
float
Progress::get_fraction () const
{
	int const t = g_atomic_int_get (&_total);
	if (t == 0) {
		return -1;
	}
	
	return float (g_atomic_int_get (&_progress)) / t;
}

bool
Progress::get_done () const
{
	return g_atomic_int_get (&_done) == 1;
}
