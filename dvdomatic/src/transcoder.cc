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

#include <sigc++/signal.h>
#include "transcoder.h"
#include "encoder.h"

Transcoder::Transcoder (Film* f, Job* j, Encoder* e, Parameters const * p)
	: _film (f)
	, _job (j)
	, _encoder (e)
	, _decoder (f, j, p)
{
	e->set_decoder (&_decoder);
	
	_decoder.Video.connect (sigc::mem_fun (*e, &Encoder::process_video));
	_decoder.Audio.connect (sigc::mem_fun (*e, &Encoder::process_audio));
}

void
Transcoder::go ()
{
	_encoder->process_begin ();
	_decoder.go ();
	_encoder->process_end ();
}
