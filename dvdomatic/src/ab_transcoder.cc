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

#include <iostream>
#include <sigc++/bind.h>
#include "ab_transcoder.h"
#include "film.h"
#include "decoder.h"
#include "encoder.h"
#include "job.h"

using namespace std;

ABTranscoder::ABTranscoder (Film* f, Job* j, Encoder* e, int w, int h, int N)
	: _film (f)
	, _job (j)
	, _encoder (e)
	, _num_frames (N)
	, _last_frame (0)
{
	Film* original = new Film (*f);
	original->set_filters (vector<Filter const *> ());
	_da = new Decoder (original, j, w, h, N);
	_db = new Decoder (f, j, w, h, N);

	/* Use the B decoder so that the encoder writes stuff to the write j2c directory */
	_encoder->set_decoder (_db);

	_da->Video.connect (sigc::bind (sigc::mem_fun (*this, &ABTranscoder::process_video), 0));
	_db->Video.connect (sigc::bind (sigc::mem_fun (*this, &ABTranscoder::process_video), 1));
	_da->Audio.connect (sigc::mem_fun (*e, &Encoder::process_audio));

	_rgb = new uint8_t[w * h * 3];
}

ABTranscoder::~ABTranscoder ()
{
	delete[] _rgb;
}

void
ABTranscoder::process_video (uint8_t* rgb, int line_size, int frame, int index)
{
	int const half_line_size = line_size / 2;

	uint8_t* p = _rgb;
	for (int y = 0; y < _da->out_height(); ++y) {
		if (index == 0) {
			memcpy (p, rgb, half_line_size);
		} else {
			memcpy (p + half_line_size, rgb + half_line_size, half_line_size);
		}

		p += line_size;
		rgb += line_size;
	}

	_encoder->process_video (_rgb, line_size, frame);
	
	_last_frame = frame;
}


void
ABTranscoder::go ()
{
	_encoder->process_begin ();
	
	while (1) {
		Decoder::PassResult a = _da->pass ();
		Decoder::PassResult b = _db->pass ();

		if (_job) {
			_job->set_progress (float (_last_frame) / _da->length_in_frames ());
		}
		
		if (a == Decoder::PASS_DONE && b == Decoder::PASS_DONE) {
			break;
		}

		if (_num_frames != 0 && _last_frame >= _num_frames) {
			break;
		}
	}

	_encoder->process_end ();
}
			    
