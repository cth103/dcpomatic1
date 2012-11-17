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

/** @file  src/decoder_factory.cc
 *  @brief A method to create an appropriate decoder for some content.
 */

#include <boost/filesystem.hpp>
#include "ffmpeg_decoder.h"
#include "imagemagick_decoder.h"
#include "film.h"
#include "external_audio_decoder.h"

using std::string;
using std::pair;
using std::make_pair;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;

pair<shared_ptr<VideoDecoder>, shared_ptr<AudioDecoder> >
decoder_factory (
	shared_ptr<Film> f, shared_ptr<const Options> o, Job* j
	)
{
	if (boost::filesystem::is_directory (f->content_path()) || f->content_type() == STILL) {
		/* A single image file, or a directory of them */
		return make_pair (
			shared_ptr<VideoDecoder> (new ImageMagickDecoder (f, o, j)),
			shared_ptr<AudioDecoder> ()
			);
	}

	shared_ptr<FFmpegDecoder> fd (new FFmpegDecoder (f, o, j));
	if (f->use_content_audio()) {
		return make_pair (fd, fd);
	}

	return make_pair (fd, shared_ptr<AudioDecoder> (new ExternalAudioDecoder (f, o, j)));
}
