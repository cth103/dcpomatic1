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

#include <stdexcept>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <tiffio.h>
#include "tiff_transcoder.h"
#include "film.h"
#include "format.h"

using namespace std;

TIFFTranscoder::TIFFTranscoder (Film const * film, Job* j, int width, int height, string const & tiffs, int N)
	: Transcoder (film, j, width, height, N)
	, _tiffs (tiffs)
{
	decode_audio (false);
}

TIFFTranscoder::~TIFFTranscoder ()
{
	
}	

void
TIFFTranscoder::process_video (uint8_t* data, int line_size)
{
	stringstream s;
	s << _tiffs << "/";
	s.width (8);
	s << setfill('0') << video_frame() << ".tiff";
	TIFF* output = TIFFOpen (s.str().c_str(), "w");
	if (output == 0) {
		stringstream e;
		e << "Could not create output TIFF file " << s.str();
		throw runtime_error (e.str().c_str());
	}
						
	TIFFSetField (output, TIFFTAG_IMAGEWIDTH, out_width ());
	TIFFSetField (output, TIFFTAG_IMAGELENGTH, out_height ());
	TIFFSetField (output, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	TIFFSetField (output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField (output, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	TIFFSetField (output, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField (output, TIFFTAG_SAMPLESPERPIXEL, 3);
	
	if (TIFFWriteEncodedStrip (output, 0, data, out_width() * out_height() * 3) == 0) {
		throw runtime_error ("Failed to write to output TIFF file");
	}

	TIFFClose (output);
}
