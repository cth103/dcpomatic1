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

/** @file  src/decoder.cc
 *  @brief Parent class for decoders of content.
 */

#include <stdint.h>
extern "C" {
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/avcodec.h>
}
#include "film.h"
#include "format.h"
#include "job.h"
#include "film_state.h"
#include "options.h"
#include "exceptions.h"
#include "image.h"
#include "util.h"
#include "log.h"
#include "decoder.h"
#include "filter.h"

using namespace std;
using namespace boost;

/** @param s FilmState of the Film.
 *  @param o Options.
 *  @param j Job that we are running within, or 0
 *  @param l Log to use.
 *  @param minimal true to do the bare minimum of work; just run through the content.  Useful for acquiring
 *  accurate frame counts as quickly as possible.  This generates no video or audio output.
 *  @param ignore_length Ignore the content's claimed length when computing progress.
 */
Decoder::Decoder (boost::shared_ptr<const FilmState> s, boost::shared_ptr<const Options> o, Job* j, Log* l, bool minimal, bool ignore_length)
	: _fs (s)
	, _opt (o)
	, _job (j)
	, _log (l)
	, _minimal (minimal)
	, _ignore_length (ignore_length)
	, _video_frame (0)
	, _buffer_src_context (0)
	, _buffer_sink_context (0)
	, _have_setup_video_filters (false)
{
	if (_opt->decode_video_frequency != 0 && _fs->length == 0) {
		throw DecodeError ("cannot do a partial decode if length == 0");
	}
}

/** Start decoding */
void
Decoder::go ()
{
	if (_job && _ignore_length) {
		_job->set_progress_unknown ();
	}
	
	while (pass () == false) {
		if (_job && !_ignore_length) {
			_job->set_progress (float (_video_frame) / decoding_frames ());
		}
	}
}
/** @return Number of frames that we will be decoding */
int
Decoder::decoding_frames () const
{
	if (_opt->num_frames > 0) {
		return _opt->num_frames;
	}
	
	return _fs->length;
}

/** Run one pass.  This may or may not generate any actual video / audio data;
 *  some decoders may require several passes to generate a single frame.
 *  @return true if we have finished processing all data; otherwise false.
 */
bool
Decoder::pass ()
{
	if (!_have_setup_video_filters) {
		setup_video_filters ();
		_have_setup_video_filters = true;
	}
	
	if (_opt->num_frames != 0 && _video_frame >= _opt->num_frames) {
		return true;
	}

	return do_pass ();
}

/** Called by subclasses to tell the world that some audio data is ready */
void
Decoder::process_audio (uint8_t* data, int channels, int size)
{
	if (_fs->audio_gain != 0) {
		float const linear_gain = pow (10, _fs->audio_gain / 20);
		uint8_t* p = data;
		int const samples = size / 2;
		switch (_fs->audio_sample_format) {
		case AV_SAMPLE_FMT_S16:
			for (int i = 0; i < samples; ++i) {
				/* XXX: assumes little-endian; also we should probably be dithering here */
				int const ou = p[0] | (p[1] << 8);
				int const os = ou >= 0x8000 ? (- 0x10000 + ou) : ou;
				int const gs = int (os * linear_gain);
				int const gu = gs > 0 ? gs : (0x10000 + gs);
				p[0] = gu & 0xff;
				p[1] = (gu & 0xff00) >> 8;
				p += 2;
			}
			break;
		default:
			assert (false);
		}
	}
	
	Audio (data, channels, size);
}

/** Called by subclasses to tell the world that some video data is ready.
 *  We do some post-processing / filtering then emit it for listeners.
 *  @param frame to decode; caller manages memory.
 */
void
Decoder::process_video (AVFrame* frame)
{
	if (_minimal) {
		++_video_frame;
		return;
	}

	/* Use FilmState::length here as our one may be wrong */
	if (_opt->decode_video_frequency != 0 && (_video_frame % (_fs->length / _opt->decode_video_frequency)) != 0) {
		++_video_frame;
		return;
	}

	if (av_vsrc_buffer_add_frame (_buffer_src_context, frame, 0) < 0) {
		throw DecodeError ("could not push buffer into filter chain.");
	}
	
	while (avfilter_poll_frame (_buffer_sink_context->inputs[0])) {
		AVFilterBufferRef* filter_buffer;
		if (av_buffersink_get_buffer_ref (_buffer_sink_context, &filter_buffer, 0) >= 0) {
			
			/* This takes ownership of filter_buffer */
			shared_ptr<Image> image (new FilterBufferImage ((PixelFormat) frame->format, filter_buffer));
			Video (image, _video_frame);
			++_video_frame;
		}
	}
}

void
Decoder::setup_video_filters ()
{
	stringstream fs;
	Size size_after_crop;
	
	if (_opt->apply_crop) {
		size_after_crop = _fs->cropped_size (native_size ());
		fs << crop_string (Position (_fs->left_crop, _fs->top_crop), size_after_crop);
	} else {
		size_after_crop = native_size ();
		fs << crop_string (Position (0, 0), size_after_crop);
	}

	if (_opt->padding) {
		int scaled_padding = floor (float(_opt->padding) * size_after_crop.width / _opt->out_size.width);
		fs << ",pad=" << (size_after_crop.width + (scaled_padding * 2)) << ":" << size_after_crop.height << ":" << scaled_padding << ":0:black";
	}
	
	string filters = Filter::ffmpeg_strings (_fs->filters).first;
	if (!filters.empty ()) {
		filters += ",";
	}

	filters += fs.str ();

	avfilter_register_all ();
	
	AVFilterGraph* graph = avfilter_graph_alloc();
	if (graph == 0) {
		throw DecodeError ("Could not create filter graph.");
	}
	
	AVFilter* buffer_src = avfilter_get_by_name("buffer");
	if (buffer_src == 0) {
		throw DecodeError ("Could not create buffer src filter");
	}
	
	AVFilter* buffer_sink = avfilter_get_by_name("buffersink");
	if (buffer_sink == 0) {
		throw DecodeError ("Could not create buffer sink filter");
	}
	
	stringstream a;
	a << native_size().width << ":"
	  << native_size().height << ":"
	  << pixel_format() << ":"
	  << time_base_numerator() << ":"
	  << time_base_denominator() << ":"
	  << sample_aspect_ratio_numerator() << ":"
	  << sample_aspect_ratio_denominator();

	int r;
	if ((r = avfilter_graph_create_filter (&_buffer_src_context, buffer_src, "in", a.str().c_str(), 0, graph)) < 0) {
		throw DecodeError ("could not create buffer source");
	}

	enum PixelFormat pixel_formats[] = { pixel_format(), PIX_FMT_NONE };
	if (avfilter_graph_create_filter (&_buffer_sink_context, buffer_sink, "out", 0, pixel_formats, graph) < 0) {
		throw DecodeError ("could not create buffer sink.");
	}

	AVFilterInOut* outputs = avfilter_inout_alloc ();
	outputs->name = av_strdup("in");
	outputs->filter_ctx = _buffer_src_context;
	outputs->pad_idx = 0;
	outputs->next = 0;

	AVFilterInOut* inputs = avfilter_inout_alloc ();
	inputs->name = av_strdup("out");
	inputs->filter_ctx = _buffer_sink_context;
	inputs->pad_idx = 0;
	inputs->next = 0;

	_log->log ("Using filter chain `" + filters + "'");
	if (avfilter_graph_parse (graph, filters.c_str(), &inputs, &outputs, 0) < 0) {
		throw DecodeError ("could not set up filter graph.");
	}

	if (avfilter_graph_config (graph, 0) < 0) {
		throw DecodeError ("could not configure filter graph.");
	}
}

