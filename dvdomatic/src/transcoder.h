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

#include "decoder.h"

class Film;
class Job;
class Encoder;
class FilmState;

/** A class which takes a FilmState and some Options, then uses those to transcode a Film.
 *  The same (FFmpeg) decoder is always used, and the encoder can be specified as a parameter
 *  to the constructor.
 */
class Transcoder
{
public:
	Transcoder (boost::shared_ptr<const FilmState> s, boost::shared_ptr<const Options> o, Job* j, Log* l, Encoder* e);

	void go ();

	/** @return Our decoder */
	Decoder* decoder () {
		return &_decoder;
	}

protected:
	/** A Job that is running this Transcoder, or 0 */
	Job* _job;
	/** The encoder that we will use */
	Encoder* _encoder;
	/** The decoder that we will use */
	Decoder _decoder;
};
