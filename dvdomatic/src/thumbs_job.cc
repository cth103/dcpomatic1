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

#include "thumbs_job.h"
#include "film.h"

using namespace std;

ThumbsJob::ThumbsJob (Film* f)
	: Job (f)
{
	
}

string
ThumbsJob::name () const
{
	stringstream s;
	s << "Update thumbs for " << _film->name();
	return s.str ();
}

void
ThumbsJob::run ()
{
	_film->update_thumbs_non_gui (this);
	set_state (FINISHED_OK);
}
