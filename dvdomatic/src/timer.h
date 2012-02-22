/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>
    Copyright (C) 2000-2007 Paul Davis

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

#ifndef DVDOMATIC_TIMER_H
#define DVDOMATIC_TIMER_H

#include <string>
#include <map>
#include <sys/time.h>

class PeriodTimer
{
public:
	PeriodTimer (std::string);
	~PeriodTimer ();
	
private:
	
	std::string _name;
	struct timeval _start;
};

class StateTimer
{
public:
	StateTimer (std::string, std::string);
	~StateTimer ();

	void set_state (std::string);

private:
	std::string _name;
	std::string _state;
	double _time;
	std::map<std::string, double> _totals;
};

#endif
