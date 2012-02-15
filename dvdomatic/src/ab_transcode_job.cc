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
	_pa = new Parameters (f->j2k_dir (), ".j2c", f->dir ("wavs"));
	_pa->out_width = f->format()->dci_width ();
	_pa->out_height = f->format()->dci_height ();
	_pa->num_frames = _num_frames;
	_pa->audio_channels = f->audio_channels ();
	_pa->audio_sample_rate = f->audio_sample_rate ();
	_pa->audio_sample_format = f->audio_sample_format ();
	_pa->frames_per_second = f->frames_per_second ();
	_pa->content = f->content ();
	_pa->left_crop = f->left_crop ();
	_pa->right_crop = f->right_crop ();
	_pa->top_crop = f->top_crop ();
	_pa->bottom_crop = f->bottom_crop ();
	
	_pb = new Parameters (*_pa);
	_pb->filters = f->filters ();
}

ABTranscodeJob::~ABTranscodeJob ()
{
	delete _pa;
	delete _pb;
}

string
ABTranscodeJob::name () const
{
	stringstream s;
	s << "A/B transcode " << _film_name;
	return s.str ();
}

void
ABTranscodeJob::run ()
{
	try {

		J2KWAVEncoder e (_pa);
		ABTranscoder w (_pa, _pb, this, &e);
		w.go ();
		set_progress (1);
		set_state (FINISHED_OK);

	} catch (runtime_error& e) {

		set_state (FINISHED_ERROR);

	}
}
