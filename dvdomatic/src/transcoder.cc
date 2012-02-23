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
#include <sigc++/signal.h>
#include "transcoder.h"
#include "encoder.h"
#include "decoder_factory.h"

using namespace std;
using namespace boost;

/** Construct a transcoder using a Decoder that we create and a supplied Encoder.
 *  @param s FilmState of Film that we are transcoding.
 *  @param o Options.
 *  @param j Job that we are running under, or 0.
 *  @param e Encoder to use.
 */
Transcoder::Transcoder (shared_ptr<const FilmState> s, shared_ptr<const Options> o, Job* j, Log* l, Encoder* e)
	: _job (j)
	, _encoder (e)
	, _decoder (decoder_factory (s, o, j, l))
{
	assert (_encoder);
	
	_decoder->Video.connect (sigc::mem_fun (*e, &Encoder::process_video));
	_decoder->Audio.connect (sigc::mem_fun (*e, &Encoder::process_audio));
}

/** Run the decoder, passing its output to the encoder, until the decoder
 *  has no more data to present.
 */
void
Transcoder::go ()
{
	_encoder->process_begin ();
	try {
		_decoder->go ();
	} catch (...) {
		/* process_end() is important as the decoder may have worker
		   threads that need to be cleaned up.
		*/
		_encoder->process_end ();
		throw;
	}

	_encoder->process_end ();
}
