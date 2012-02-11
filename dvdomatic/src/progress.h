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

#ifndef DVDOMATIC_PROGRESS_H
#define DVDOMATIC_PROGRESS_H

#include <list>
#include <boost/thread/mutex.hpp>

class Progress
{
public:
	Progress ();
	
	void set_progress (float);

	void ascend ();
	void descend (float);

	float get_overall_progress () const;

private:
	boost::mutex _mutex;
	
	struct Level {
		Level (float a) : allocation (a), normalised (0) {}

		float allocation;
		float normalised;
	};

	std::list<Level> _stack;
};

#endif
