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

#include <stdint.h>
#include "film.h"
#include "format.h"
#include "job.h"
#include "film_state.h"
#include "options.h"
#include "exceptions.h"
#include "image.h"
#include "util.h"
#include "log.h"
#include "decoder.h"

using namespace std;
using namespace boost;

Decoder::Decoder (boost::shared_ptr<const FilmState> s, boost::shared_ptr<const Options> o, Job* j, Log* l, bool minimal, bool ignore_length)
	: _fs (s)
	, _opt (o)
	, _job (j)
	, _log (l)
	, _minimal (minimal)
	, _ignore_length (ignore_length)
	, _video_frame (0)
{
	
}

Decoder::~Decoder ()
{
	
}	

void
Decoder::go ()
{
	if (_job && _ignore_length) {
		_job->set_progress_unknown ();
	}
	
	while (pass () != PASS_DONE) {
		if (_job && !_ignore_length) {
			_job->set_progress (float (_video_frame) / decoding_frames ());
		}
	}
}
/** @return Number of frames that we will be decoding */
int
Decoder::decoding_frames () const
{
	if (_opt->num_frames > 0) {
		return _opt->num_frames;
	}
	
	return _fs->length;
}
