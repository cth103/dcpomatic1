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

#include <stdexcept>
#include "ab_transcode_job.h"
#include "j2k_wav_encoder.h"
#include "film.h"
#include "format.h"
#include "ab_transcoder.h"
#include "film_state.h"

using namespace std;

/** @param s FilmState to compare (with filters and/or a non-bicubic scaler).
 *  @param o Options.
 *  @Param l A log that we can write to.
 */
ABTranscodeJob::ABTranscodeJob (boost::shared_ptr<const FilmState> s, boost::shared_ptr<const Options> o, Log* l)
	: Job (s, o, l)
{
	_fs_b.reset (new FilmState (*_fs));
	_fs_b->filters.clear ();
	/* This is somewhat arbitrary, but hey ho */
	_fs_b->scaler = Scaler::get_from_id ("bicubic");
}

string
ABTranscodeJob::name () const
{
	stringstream s;
	s << "A/B transcode " << _fs->name;
	return s.str ();
}

void
ABTranscodeJob::run ()
{
	try {

		J2KWAVEncoder e (_fs, _opt);
		/* _fs_b is the one with no filters */
		ABTranscoder w (_fs_b, _fs, _opt, this, &e);
		w.go ();
		set_progress (1);
		set_state (FINISHED_OK);

	} catch (exception& e) {

		set_state (FINISHED_ERROR);

	}
}
