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

#include <vector>
#include <string>
extern "C" {
#include <libavcodec/avcodec.h>
}
#include <sndfile.h>
#include "decoder.h"

struct AVFilterGraph;
struct AVCodecContext;
struct AVFilterContext;
struct AVFormatContext;
struct AVFrame;
struct AVBufferContext;
struct AVCodec;
class Film;
class Progress;

class FilmWriter : public Decoder
{
public:
	FilmWriter (Film const *, Progress *, int, int, std::string const &, std::string const &, int N = 0);
	~FilmWriter ();

	void go ();

private:

	void process_video (uint8_t *, int);
	void process_audio (uint8_t *, int, int);

	void write_tiff (std::string const &, int, uint8_t *, int, int) const;
	void decode ();
	
	std::string _tiffs;
	std::string _wavs;
	Progress* _progress;
	int _nframes;

	std::vector<SNDFILE*> _sound_files;
	int _deinterleave_buffer_size;
	uint8_t* _deinterleave_buffer;
};
