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

using namespace std;

ThumbsJob::ThumbsJob (Film* f)
	: Job (f)
{
	_par = new Parameters (f->dir ("thumbs"), ".tiff", "");

	_par->out_width = f->width ();
	_par->out_height = f->height ();
	_par->apply_crop = false;
	_par->decode_audio = false;
	_par->decode_video_frequency = 128;
	_par->frames_per_second = f->frames_per_second ();
}

ThumbsJob::~ThumbsJob ()
{
	delete _par;
}

string
ThumbsJob::name () const
{
	stringstream s;
	s << "Update thumbs for " << _film_name;
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
