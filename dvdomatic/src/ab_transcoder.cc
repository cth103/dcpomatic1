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
#include "image.h"

using namespace std;
using namespace boost;

ABTranscoder::ABTranscoder (
	boost::shared_ptr<const FilmState> a, boost::shared_ptr<const FilmState> b, boost::shared_ptr<const Options> o, Job* j, Log* l, Encoder* e)
	: _fs_a (a)
	, _fs_b (b)
	, _opt (o)
	, _job (j)
	, _log (l)
	, _encoder (e)
	, _last_frame (0)
{
	_da = new Decoder (_fs_a, o, j, _log);
	_db = new Decoder (_fs_b, o, j, _log);

	_da->Video.connect (sigc::bind (sigc::mem_fun (*this, &ABTranscoder::process_video), 0));
	_db->Video.connect (sigc::bind (sigc::mem_fun (*this, &ABTranscoder::process_video), 1));
	_da->Audio.connect (sigc::mem_fun (*e, &Encoder::process_audio));
}

ABTranscoder::~ABTranscoder ()
{

}

void
ABTranscoder::process_video (shared_ptr<Image> yuv, int frame, int index)
{
	if (index == 0) {
		/* Keep this image around until we get the other half */
		_image = yuv;
	} else {
		/* Copy the right half of yuv into _image */
		for (int i = 0; i < yuv->components(); ++i) {
			int const line_size = yuv->line_size()[i];
			int const half_line_size = line_size / 2;

			uint8_t* p = _image->data()[i];
			uint8_t* q = yuv->data()[i];
			
			for (int j = 0; j < yuv->lines (i); ++j) {
				memcpy (p + half_line_size, q + half_line_size, half_line_size);
				p += line_size;
				q += line_size;
			}
			
			/* And pass it to the encoder */
			_encoder->process_video (_image, frame);
			_image.reset ();
		}
	}
	
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
			_job->set_progress (float (_last_frame) / _da->decoding_frames ());
		}
		
		if (a == Decoder::PASS_DONE && b == Decoder::PASS_DONE) {
			break;
		}
	}

	_encoder->process_end ();
}
			    
