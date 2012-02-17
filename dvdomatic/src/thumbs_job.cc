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
#include "parameters.h"
#include "tiff_encoder.h"
#include "transcoder.h"
#include "options.h"

using namespace std;

ThumbsJob::ThumbsJob (Parameters const * p, Options const * o, Log* l)
	: Job (p, o, l)
{
	
}

string
ThumbsJob::name () const
{
	stringstream s;
	s << "Update thumbs for " << _par->film_name;
	return s.str ();
}

void
ThumbsJob::run ()
{
	TIFFEncoder e (_par);
	Transcoder w (_par, this, &e);
	w.go ();
	set_progress (1);
	set_state (FINISHED_OK);
}
