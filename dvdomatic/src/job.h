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

#ifndef DVDOMATIC_JOB_H
#define DVDOMATIC_JOB_H

#include <string>
#include <sigc++/sigc++.h>
#include "progress.h"

class Film;

class Job
{
public:
	Job (Film *);

	virtual std::string name () const = 0;
	virtual void run () = 0;
	
	void start ();

	bool running () const;
	bool finished () const;
	bool finished_ok () const;
	bool finished_in_error () const;
	float progress () const;

	int elapsed_time () const;

	void emit_finished ();

	/** Emitted from the GUI thread */
	sigc::signal0<void> Finished;

protected:

	enum State {
		NEW,
		RUNNING,
		FINISHED_OK,
		FINISHED_ERROR
	};
	
	void set_state (State);
	
	Film* _film;
	Progress _progress;
	std::string _name;

private:
	
	mutable boost::mutex _state_mutex;
	State _state;
	time_t _start_time;
};

#endif
