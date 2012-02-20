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

#include <boost/shared_ptr.hpp>
#include <stdint.h>

class FilmState;
class Options;

/** Parent class for classes which can encode video and audio frames.
 *  Video is supplied to process_video as RGB frames, and audio
 *  is supplied as uncompressed PCM in blocks of various sizes.
 *
 *  The subclass is expected to encode the video and/or audio in
 *  some way and write it to disk.
 */
class Encoder
{
public:
	Encoder (boost::shared_ptr<const FilmState>, boost::shared_ptr<const Options>);

	virtual void process_begin () = 0;
	virtual void process_video (uint8_t **, int *, int, int, int) = 0;
	virtual void process_audio (uint8_t *, int, int) = 0;
	virtual void process_end () = 0;

protected:
	boost::shared_ptr<const FilmState> _fs;
	boost::shared_ptr<const Options> _opt;
};
