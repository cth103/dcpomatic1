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

#include <string>
#include <sndfile.h>
#include "transcoder.h"

class Film;
class Job;

class TIFFTranscoder : public Transcoder
{
public:
	TIFFTranscoder (Film const *, Job *, int, int, std::string const &, int N = 0);
	~TIFFTranscoder ();

private:

	void process_video (uint8_t *, int);
	void process_audio (uint8_t *, int, int) {}

	std::string _tiffs;
};
