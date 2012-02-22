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
#include <sys/time.h>
#include "timer.h"
#include "util.h"

using namespace std;

PeriodTimer::PeriodTimer (string n)
	: _name (n)
{
	gettimeofday (&_start, 0);
}

PeriodTimer::~PeriodTimer ()
{
	struct timeval stop;
	gettimeofday (&stop, 0);
	cout << "T: " << _name << ": " << (seconds (stop) - seconds (_start)) << "\n";
}

	
StateTimer::StateTimer (string n, string s)
	: _name (n)
{
	struct timeval t;
	gettimeofday (&t, 0);
	_time = seconds (t);
	_state = s;
}

void
StateTimer::set_state (string s)
{
	double const last = _time;
	struct timeval t;
	gettimeofday (&t, 0);
	_time = seconds (t);

	if (_totals.find (s) == _totals.end ()) {
		_totals[s] = 0;
	}

	_totals[_state] += _time - last;
	_state = s;
}

StateTimer::~StateTimer ()
{
	if (_state.empty ()) {
		return;
	}

	
	set_state ("");

	cout << _name << ":\n";
	for (map<string, double>::iterator i = _totals.begin(); i != _totals.end(); ++i) {
		cout << "\t" << i->first << " " << i->second << "\n";
	}
}
