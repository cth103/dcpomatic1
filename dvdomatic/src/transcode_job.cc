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
#include "j2k_wav_encoder.h"
#include "film.h"
#include "format.h"
#include "transcoder.h"
#include "parameters.h"
#include "log.h"

using namespace std;

TranscodeJob::TranscodeJob (Film* f, int N)
	: Job (f)
	, _num_frames (N)
{
	_par = new Parameters (f->j2k_dir(), ".j2c", f->dir ("wavs"));
	_par->out_width = f->format()->dci_width ();
	_par->out_height = f->format()->dci_height ();
	_par->num_frames = _num_frames;
	_par->audio_channels = f->audio_channels ();
	_par->audio_sample_rate = f->audio_sample_rate ();
	_par->audio_sample_format = f->audio_sample_format ();
	_par->frames_per_second = f->frames_per_second ();
	_par->content = f->content ();
	_par->left_crop = f->left_crop ();
	_par->right_crop = f->right_crop ();
	_par->top_crop = f->top_crop ();
	_par->bottom_crop = f->bottom_crop ();
	_par->filters = f->filters ();
}

TranscodeJob::~TranscodeJob ()
{
	delete _par;
}

string
TranscodeJob::name () const
{
	stringstream s;
	s << "Transcode " << _film_name;
	return s.str ();
}

void
TranscodeJob::run ()
{
	try {

		_log->log ("Transcode job starting");
		_log->log (_par->summary ());

		J2KWAVEncoder e (_par);
		Transcoder w (_par, this, &e);
		w.go ();
		set_progress (1);
		set_state (FINISHED_OK);

		_log->log ("Transcode job completed successfully");

	} catch (runtime_error& e) {

		stringstream s;
		s << "Transcode job failed (" << e.what() << ")";
		_log->log (s.str ());
		set_state (FINISHED_ERROR);

	}
}
