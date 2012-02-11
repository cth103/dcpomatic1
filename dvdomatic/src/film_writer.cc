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
extern "C" {
#include <tiffio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
}
#include <sndfile.h>
#include "film_writer.h"
#include "film.h"
#include "format.h"
#include "progress.h"

using namespace std;

FilmWriter::FilmWriter (Film const * film, Progress* p, int width, int height, string const & tiffs, string const & wavs, int N)
	: Decoder (film, width, height)
	, _tiffs (tiffs)
	, _wavs (wavs)
	, _progress (p)
	, _nframes (N)
	, _deinterleave_buffer_size (8192)
	, _deinterleave_buffer (0)
{
	/* Create sound output files */
	for (int i = 0; i < audio_channels(); ++i) {
		stringstream wav_path;
		wav_path << _wavs << "/" << (i + 1) << ".wav";
		SF_INFO sf_info;
		sf_info.samplerate = audio_sample_rate();
		/* We write mono files */
		sf_info.channels = 1;
		sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
		SNDFILE* f = sf_open (wav_path.str().c_str(), SFM_WRITE, &sf_info);
		if (f == 0) {
			throw runtime_error ("Could not create audio output file");
		}
		_sound_files.push_back (f);
	}

	/* Create buffer for deinterleaving audio */
	_deinterleave_buffer = new uint8_t[_deinterleave_buffer_size];
}

void
FilmWriter::go ()
{
	decode ();
}

FilmWriter::~FilmWriter ()
{
	delete[] _deinterleave_buffer;

	for (vector<SNDFILE*>::iterator i = _sound_files.begin(); i != _sound_files.end(); ++i) {
		sf_close (*i);
	}
}	

void
FilmWriter::decode ()
{
	while (pass () != PASS_DONE && (_nframes == 0 || video_frame() < _nframes)) {
		_progress->set_progress (float (video_frame()) / length_in_frames ());
		/* Decoder will call our decode_{video,audio} methods */
	}
}

void
FilmWriter::process_video (uint8_t* data, int line_size)
{
	write_tiff (_tiffs, video_frame(), data, out_width(), out_height());
}

void
FilmWriter::process_audio (uint8_t* data, int channels, int data_size)
{
	/* Size of a sample in bytes */
	int const sample_size = 2;
	
	/* XXX: we are assuming that sample_size is right, the _deinterleave_buffer_size is a multiple
	   of the sample size and that data_size is a multiple of channels * sample_size.
	*/
	
	/* XXX: this code is very tricksy and it must be possible to make it simpler ... */
	
	/* Number of bytes left to read this time */
	int remaining = data_size;
	/* Our position in the output buffers, in bytes */
	int position = 0;
	while (remaining > 0) {
		/* How many bytes of the deinterleaved data to do this time */
		int this_time = min (remaining / channels, _deinterleave_buffer_size);
		for (int i = 0; i < channels; ++i) {
			for (int j = 0; j < this_time; j += sample_size) {
				for (int k = 0; k < sample_size; ++k) {
					int const to = j + k;
					int const from = position + (i * sample_size) + (j * channels) + k;
					_deinterleave_buffer[to] = data[from];
				}
			}
			
			switch (audio_sample_format ()) {
			case AV_SAMPLE_FMT_S16:
				sf_write_short (_sound_files[i], (const short *) _deinterleave_buffer, this_time / sample_size);
				break;
			default:
				throw runtime_error ("Unknown audio sample format");
			}
		}
		
		position += this_time;
		remaining -= this_time * channels;
	}
}

void
FilmWriter::write_tiff (string const & dir, int frame, uint8_t* data, int w, int h) const
{
	stringstream s;
	s << dir << "/";
	s.width (8);
	s << setfill('0') << frame << ".tiff";
	TIFF* output = TIFFOpen (s.str().c_str(), "w");
	if (output == 0) {
		stringstream e;
		e << "Could not create output TIFF file " << s.str();
		throw runtime_error (e.str().c_str());
	}
						
	TIFFSetField (output, TIFFTAG_IMAGEWIDTH, w);
	TIFFSetField (output, TIFFTAG_IMAGELENGTH, h);
	TIFFSetField (output, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	TIFFSetField (output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField (output, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	TIFFSetField (output, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField (output, TIFFTAG_SAMPLESPERPIXEL, 3);
	
	if (TIFFWriteEncodedStrip (output, 0, data, w * h * 3) == 0) {
		throw runtime_error ("Failed to write to output TIFF file");
	}

	TIFFClose (output);
}
