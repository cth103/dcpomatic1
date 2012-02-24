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

/** @file src/tiff_encoder.h
 *  @brief An encoder that writes TIFF files (and does nothing with audio).
 */

#include <string>
#include <sndfile.h>
#include "encoder.h"

class FilmState;
class Log;

/** An encoder that writes TIFF files (and does nothing with audio) */
class TIFFEncoder : public Encoder
{
public:
	TIFFEncoder (boost::shared_ptr<const FilmState> s, boost::shared_ptr<const Options> o, Log* l);

	void process_begin () {}
	void process_video (boost::shared_ptr<Image>, int);
	void process_audio (uint8_t *, int, int) {}
	void process_end () {}
};
