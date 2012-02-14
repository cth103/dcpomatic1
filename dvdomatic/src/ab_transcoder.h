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

#include <stdint.h>

class Film;
class Job;
class Encoder;
class Decoder;

class ABTranscoder
{
public:
	ABTranscoder (Film *, Job *, Encoder *, int, int, int N = 0);
	~ABTranscoder ();

	void go ();

private:
	void process_video (uint8_t *, int, int, int);
	
	Film* _film;
	Job* _job;
	Encoder* _encoder;
	int _num_frames;
	Decoder* _da;
	Decoder* _db;
	uint8_t* _rgb;
	int _last_frame;
};
