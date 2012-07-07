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
#include "tiff_decoder.h"
#include "imagemagick_decoder.h"
#include "film_state.h"

using namespace std;
using namespace boost;

shared_ptr<Decoder>
decoder_factory (
	shared_ptr<const FilmState> fs, shared_ptr<const Options> o, Job* j, Log* l, bool minimal = false, bool ignore_length = false
	)
{
	if (filesystem::is_directory (fs->content_path ())) {
		/* Assume a directory contains TIFFs */
		return shared_ptr<Decoder> (new TIFFDecoder (fs, o, j, l, minimal, ignore_length));
	}

#if BOOST_FILESYSTEM_VERSION == 3
	string const ext = filesystem::path(fs->content_path()).extension().string();
#else
	string const ext = filesystem::path(fs->content_path()).extension();
#endif
	if (ext == ".tif" || ext == ".tiff" || ext == ".jpg" || ext == ".jpeg" || ext == ".png") {
		return shared_ptr<Decoder> (new ImageMagickDecoder (fs, o, j, l, minimal, ignore_length));
	}
	
	return shared_ptr<Decoder> (new FFmpegDecoder (fs, o, j, l, minimal, ignore_length));
}
