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
#include <boost/shared_ptr.hpp>
#include <sigc++/bind.h>
#include "ab_transcoder.h"
#include "film.h"
#include "decoder.h"
#include "encoder.h"
#include "job.h"
#include "film_state.h"
#include "options.h"

using namespace std;
using namespace boost;

ABTranscoder::ABTranscoder (boost::shared_ptr<const FilmState> a, boost::shared_ptr<const FilmState> b, boost::shared_ptr<const Options> o, Job* j, Encoder* e)
	: _fs_a (a)
	, _fs_b (b)
	, _opt (o)
	, _job (j)
	, _encoder (e)
	, _last_frame (0)
{
	_da = new Decoder (_fs_a, o, j);
	_db = new Decoder (_fs_b, o, j);

	_da->Video.connect (sigc::bind (sigc::mem_fun (*this, &ABTranscoder::process_video), 0));
	_db->Video.connect (sigc::bind (sigc::mem_fun (*this, &ABTranscoder::process_video), 1));
	_da->Audio.connect (sigc::mem_fun (*e, &Encoder::process_audio));
}

ABTranscoder::~ABTranscoder ()
{
	delete[] _rgb;
}

void
ABTranscoder::process_video (shared_ptr<Image> yuv, int frame, int index)
{
#if 0	
	int const half_line_size = line_size / 2;

	uint8_t* p = _rgb;
	for (int y = 0; y < _opt->out_height; ++y) {
		if (index == 0) {
			memcpy (p, rgb, half_line_size);
		} else {
			memcpy (p + half_line_size, rgb + half_line_size, half_line_size);
		}

		p += line_size;
		rgb += line_size;
	}

	if (index == 1) {
		/* Now we have a complete frame, so pass it on to the encoder */
		_encoder->process_video (_rgb, line_size, frame);
	}
	
	_last_frame = frame;
#endif	
}


void
ABTranscoder::go ()
{
	_encoder->process_begin ();
	
	while (1) {
		Decoder::PassResult a = _da->pass ();
		Decoder::PassResult b = _db->pass ();

		if (_job) {
			_job->set_progress (float (_last_frame) / _da->decoding_frames ());
		}
		
		if (a == Decoder::PASS_DONE && b == Decoder::PASS_DONE) {
			break;
		}
	}

	_encoder->process_end ();
}
			    
