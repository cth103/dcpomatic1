/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

extern "C" {
#include "libavutil/channel_layout.h"
}	
#include "resampler.h"
#include "audio_buffers.h"
#include "exceptions.h"

#include "i18n.h"

using std::cout;
using boost::shared_ptr;

Resampler::Resampler (int in, int out, int channels)
	: _in_rate (in)
	, _out_rate (out)
	, _channels (channels)
{
	/* We will be using planar float data when we call the
	   resampler.  As far as I can see, the audio channel
	   layout is not necessary for our purposes; it seems
	   only to be used get the number of channels and
	   decide if rematrixing is needed.  It won't be, since
	   input and output layouts are the same.
	*/

	cout << "resamp for " << _channels << " " << _in_rate << " " << _out_rate << "\n";

	_swr_context = swr_alloc_set_opts (
		0,
		av_get_default_channel_layout (_channels),
		AV_SAMPLE_FMT_FLTP,
		_out_rate,
		av_get_default_channel_layout (_channels),
		AV_SAMPLE_FMT_FLTP,
		_in_rate,
		0, 0
		);
	
	swr_init (_swr_context);
}

Resampler::~Resampler ()
{
	swr_free (&_swr_context);
}

shared_ptr<const AudioBuffers>
Resampler::run (shared_ptr<const AudioBuffers> in)
{
	/* Compute the resampled frames count and add 32 for luck */
	int const max_resampled_frames = ceil ((double) in->frames() * _out_rate / _in_rate) + 32;
	shared_ptr<AudioBuffers> resampled (new AudioBuffers (_channels, max_resampled_frames));

	int const resampled_frames = swr_convert (
		_swr_context, (uint8_t **) resampled->data(), max_resampled_frames, (uint8_t const **) in->data(), in->frames()
		);
	
	if (resampled_frames < 0) {
		throw EncodeError (_("could not run sample-rate converter"));
	}
	
	resampled->set_frames (resampled_frames);
	return resampled;
}	

shared_ptr<const AudioBuffers>
Resampler::flush ()
{
	shared_ptr<AudioBuffers> out (new AudioBuffers (_channels, 0));
	int out_offset = 0;
	int64_t const pass_size = 256;
	shared_ptr<AudioBuffers> pass (new AudioBuffers (_channels, 256));

	while (1) {
		int const frames = swr_convert (_swr_context, (uint8_t **) pass->data(), pass_size, 0, 0);
		
		if (frames < 0) {
			throw EncodeError (_("could not run sample-rate converter"));
		}
		
		if (frames == 0) {
			break;
		}

		out->ensure_size (out_offset + frames);
		out->copy_from (pass.get(), frames, 0, out_offset);
		out_offset += frames;
		out->set_frames (out_offset);
	}

	return out;
}
