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

#ifndef DVDOMATIC_TIFF_DECODER_H
#define DVDOMATIC_TIFF_DECODER_H

#include <vector>
#include <string>
#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include "util.h"
#include "decoder.h"

class Job;
class FilmState;
class Options;
class Image;
class Log;

class TIFFDecoder : public Decoder
{
public:
	TIFFDecoder (boost::shared_ptr<const FilmState>, boost::shared_ptr<const Options>, Job *, Log *, bool, bool);
	~TIFFDecoder ();

	/* Methods to query our input video */
	int length_in_frames () const;
	float frames_per_second () const;
	Size native_size () const;
	int audio_channels () const;
	int audio_sample_rate () const;
	AVSampleFormat audio_sample_format () const;

	PassResult do_pass ();

private:
	std::string file_path (std::string) const;
	std::list<std::string> _files;
	std::list<std::string>::iterator _iter;
};

#endif
