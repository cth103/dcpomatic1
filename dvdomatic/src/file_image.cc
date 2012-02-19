/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>
    Taken from code Copyright (C) 2010-2011 Terrence Meiczinger

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

#include <cstdio>
#include <boost/filesystem.hpp>
#include "options.h"
#include "exceptions.h"
#include "file_image.h"

using namespace std;
using namespace boost;

FileImage::FileImage (shared_ptr<const Options> o, uint8_t* rgb, int f, int w, int h, int fps)
	: Image (rgb, f, w, h, fps)
	, _opt (o)
{
	
}

void
FileImage::output (uint8_t* data, int length)
{
	string const tmp_j2k = _opt->frame_out_path (_frame, true);

	FILE* f = fopen (tmp_j2k.c_str (), "wb");
	
	if (!f) {
		throw WriteFileError (tmp_j2k);
	}

	fwrite (data, 1, length, f);
	fclose (f);

	/* Rename the file from foo.j2c.tmp to foo.j2c now that it is complete */
	filesystem::rename (tmp_j2k, _opt->frame_out_path (_frame, false));
}
