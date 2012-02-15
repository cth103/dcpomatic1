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
#include "parameters.h"

using namespace std;

ABTranscodeJob::ABTranscodeJob (Film* f, int N)
	: Job (f)
	, _num_frames (N)
{

}

string
ABTranscodeJob::name () const
{
	stringstream s;
	s << "A/B transcode " << _film->name();
	return s.str ();
}

void
ABTranscodeJob::run ()
{
	try {

		Parameters p;
		p.out_width = _film->format()->dci_width ();
		p.out_height = _film->format()->dci_height ();
		p.num_frames = _num_frames;
		
		J2KWAVEncoder e (_film, &p);
		ABTranscoder w (_film, &p, this, &e);
		w.go ();
		set_progress (1);
		set_state (FINISHED_OK);

	} catch (runtime_error& e) {

		set_state (FINISHED_ERROR);

	}
}
