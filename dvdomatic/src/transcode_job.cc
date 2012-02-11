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
#include "transcode_job.h"
#include "j2k_wav_transcoder.h"
#include "film.h"
#include "format.h"

using namespace std;

TranscodeJob::TranscodeJob (Film* f)
	: Job (f)
{

}

string
TranscodeJob::name () const
{
	stringstream s;
	s << "Transcode " << _film->name();
	return s.str ();
}

void
TranscodeJob::run ()
{
	try {

		J2KWAVTranscoder w (_film, &_progress, _film->format()->dci_width(), _film->format()->dci_height());
		w.go ();
		set_state (FINISHED_OK);

	} catch (runtime_error& e) {

		set_state (FINISHED_ERROR);

	}
}
