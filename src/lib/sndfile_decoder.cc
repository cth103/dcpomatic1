/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

#include <iostream>
#ifdef DCPOMATIC_WINDOWS
#include <windows.h>
#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1
#endif
#include <sndfile.h>
#include "sndfile_content.h"
#include "sndfile_decoder.h"
#include "film.h"
#include "exceptions.h"
#include "audio_buffers.h"

#include "i18n.h"

using std::vector;
using std::string;
using std::min;
using std::cout;
using std::map;
using boost::shared_ptr;

SndfileDecoder::SndfileDecoder (shared_ptr<const Film> f, shared_ptr<const SndfileContent> c)
	: Decoder (f)
	, AudioDecoder (f, c)
	, _sndfile_content (c)
	, _deinterleave_buffer (0)
{
	_info.format = 0;

	/* Here be monsters.  See fopen_boost for similar shenanigans */
#ifdef DCPOMATIC_WINDOWS
	_sndfile = sf_wchar_open (_sndfile_content->path(0).c_str(), SFM_READ, &_info);
#else	
	_sndfile = sf_open (_sndfile_content->path(0).string().c_str(), SFM_READ, &_info);
#endif
	
	if (!_sndfile) {
		throw DecodeError (String::compose (_("could not open audio file for reading (%1)"), sf_strerror (0)));
	}

	_done = 0;
	_remaining = _info.frames;
}

SndfileDecoder::~SndfileDecoder ()
{
	sf_close (_sndfile);
	delete[] _deinterleave_buffer;
}

void
SndfileDecoder::pass ()
{
	/* Do things in half second blocks as I think there may be limits
	   to what FFmpeg (and in particular the resampler) can cope with.
	*/
	sf_count_t const block = _sndfile_content->audio_stream()->frame_rate() / 2;
	sf_count_t const this_time = min (block, _remaining);

	int const channels = _sndfile_content->audio_channels ();
	
	shared_ptr<AudioBuffers> data (new AudioBuffers (channels, this_time));

	if (_sndfile_content->audio_channels() == 1) {
		/* No de-interleaving required */
		sf_read_float (_sndfile, data->data(0), this_time);
	} else {
		/* Deinterleave */
		if (!_deinterleave_buffer) {
			_deinterleave_buffer = new float[block * channels];
		}
		sf_readf_float (_sndfile, _deinterleave_buffer, this_time);
		vector<float*> out_ptr (channels);
		for (int i = 0; i < channels; ++i) {
			out_ptr[i] = data->data(i);
		}
		float* in_ptr = _deinterleave_buffer;
		for (int i = 0; i < this_time; ++i) {
			for (int j = 0; j < channels; ++j) {
				*out_ptr[j]++ = *in_ptr++;
			}
		}
	}
		
	data->set_frames (this_time);
	audio (data, _sndfile_content->audio_stream(), _done);
	_done += this_time;
	_remaining -= this_time;
}

int
SndfileDecoder::audio_channels () const
{
	return _info.channels;
}

AudioContent::Frame
SndfileDecoder::audio_length () const
{
	return _info.frames;
}

int
SndfileDecoder::audio_frame_rate () const
{
	return _info.samplerate;
}

bool
SndfileDecoder::done () const
{
	map<AudioStreamPtr, AudioContent::Frame>::const_iterator i = _audio_position.find (_sndfile_content->audio_stream ());
	if (i == _audio_position.end ()) {
		return false;
	}
	return i->second >= _sndfile_content->audio_length ();
}
