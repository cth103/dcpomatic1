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

float
Progress::get_fraction () const
{
	return float (g_atomic_int_get (&_progress)) / g_atomic_int_get (&_total);
}
