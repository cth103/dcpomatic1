/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

/** @file  src/transcoder.cc
 *  @brief A class which takes a Film and some Options, then uses those to transcode the film.
 *
 *  A decoder is selected according to the content type, and the encoder can be specified
 *  as a parameter to the constructor.
 */

#include <iostream>
#include <boost/signals2.hpp>
#include "transcoder.h"
#include "encoder.h"
#include "film.h"
#include "video_decoder.h"
#include "audio_decoder.h"
#include "player.h"
#include "job.h"

using std::string;
using std::cout;
using std::list;
using boost::shared_ptr;
using boost::weak_ptr;
using boost::dynamic_pointer_cast;

/** Construct a transcoder using a Decoder that we create and a supplied Encoder.
 *  @param f Film that we are transcoding.
 *  @param e Encoder to use.
 */
Transcoder::Transcoder (shared_ptr<const Film> f, shared_ptr<Job> j)
	: _film (f)
	, _player (f->make_player ())
	, _encoder (new Encoder (f, j))
	, _finishing (false)
{

}

void
Transcoder::go ()
{
	_encoder->process_begin ();

	DCPTime const frame = DCPTime::from_frames (1, _film->video_frame_rate ());
	for (DCPTime t; t < _film->length(); t += frame) {
		list<shared_ptr<DCPVideo> > v = _player->get_video (t, true);
		for (list<shared_ptr<DCPVideo> >::const_iterator i = v.begin(); i != v.end(); ++i) {
			_encoder->process_video (*i);
		}
		_encoder->process_audio (_player->get_audio (t, frame, true));
	}

	_finishing = true;
	_encoder->process_end ();

	_player->statistics().dump (_film->log ());
}

float
Transcoder::current_encoding_rate () const
{
	return _encoder->current_encoding_rate ();
}

int
Transcoder::video_frames_out () const
{
	return _encoder->video_frames_out ();
}

