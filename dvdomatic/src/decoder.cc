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
#include <stdint.h>
extern "C" {
#include <tiffio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libpostproc/postprocess.h>
}
#include <sndfile.h>
#include "film.h"
#include "format.h"
#include "transcoder.h"
#include "job.h"
#include "filter.h"
#include "parameters.h"
#include "options.h"

using namespace std;

Decoder::Decoder (Parameters const * p, Options const * o, Job* j)
	: _par (p)
	, _opt (o)
	, _job (j)
	, _format_context (0)
	, _video_stream (-1)
	, _audio_stream (-1)
	, _frame_in (0)
	, _frame_out (0)
	, _video_codec_context (0)
	, _video_codec (0)
	, _frame_out_buffer (0)
	, _buffer_src_context (0)
	, _buffer_sink_context (0)
	, _conversion_context (0)
	, _audio_codec_context (0)
	, _audio_codec (0)
	, _video_frame (0)
	, _post_filter_width (0)
	, _post_filter_height (0)
	, _pp_mode (0)
	, _pp_context (0)
	, _pp_buffer (0)
{
	setup_general ();
	setup_video ();
	setup_audio ();
}

Decoder::~Decoder ()
{
	sws_freeContext (_conversion_context);

	if (_audio_codec_context) {
		avcodec_close (_audio_codec_context);
	}
	
	av_free (_frame_out_buffer);
	
	if (_video_codec_context) {
		avcodec_close (_video_codec_context);
	}
	
	av_free (_frame_out);
	av_free (_frame_in);
	avformat_close_input (&_format_context);

	if (_pp_mode) {
		pp_free_mode (_pp_mode);
	}

	if (_pp_context) {
		pp_free_context (_pp_context);
	}

	if (_pp_buffer) {
		for (int i = 0; i < 4; ++i) {
			delete[] _pp_buffer[i];
		}
	}

	delete[] _pp_buffer;
}	

void
Decoder::setup_general ()
{
	int r;
	
	av_register_all ();

	if ((r = avformat_open_input (&_format_context, _par->content.c_str(), 0, 0)) != 0) {
		stringstream s;
		s << "could not open content file " << _par->content << " (" << r << ")";
		throw runtime_error (s.str ());
	}

	if (avformat_find_stream_info (_format_context, 0) < 0) {
		throw runtime_error ("Could not find stream information");
	}

	for (uint32_t i = 0; i < _format_context->nb_streams; ++i) {
		if (_format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			_video_stream = i;
		} else if (_format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			_audio_stream = i;
		}
	}

	if (_video_stream < 0) {
		throw runtime_error ("Could not find video stream");
	}
	if (_audio_stream < 0) {
		throw runtime_error ("Could not find audio stream");
	}

	_frame_in = avcodec_alloc_frame ();
	if (_frame_in == 0) {
		throw runtime_error ("Could not allocate frame");
	}

	_frame_out = avcodec_alloc_frame ();
	if (_frame_out == 0) {
		throw runtime_error ("Could not allocate frame");
	}
}

void
Decoder::setup_video ()
{
	_video_codec_context = _format_context->streams[_video_stream]->codec;
	_video_codec = avcodec_find_decoder (_video_codec_context->codec_id);

	if (_video_codec == 0) {
		throw runtime_error ("Could not find video decoder");
	}
	
	if (avcodec_open2 (_video_codec_context, _video_codec, 0) < 0) {
		throw runtime_error ("Could not open video decoder");
	}

	int num_bytes = avpicture_get_size (PIX_FMT_RGB24, _opt->out_width, _opt->out_height);
	_frame_out_buffer = (uint8_t *) av_malloc (num_bytes);

	avpicture_fill ((AVPicture *) _frame_out, _frame_out_buffer, PIX_FMT_RGB24, _opt->out_width, _opt->out_height);

	_post_filter_width = _video_codec_context->width;
	_post_filter_height = _video_codec_context->height;
	
	if (_par->apply_crop) {
		_post_filter_width -= _par->left_crop + _par->right_crop;
		_post_filter_height -= _par->top_crop + _par->bottom_crop;		
	}

	setup_post_process_filters ();

	_conversion_context = sws_getContext (
		_post_filter_width, _post_filter_height, _video_codec_context->pix_fmt,
		_opt->out_width, _opt->out_height, PIX_FMT_RGB24,
		SWS_BICUBIC, 0, 0, 0
		);

	if (_conversion_context == 0) {
		throw runtime_error ("Could not obtain YUV -> RGB conversion context");
	}

	setup_video_filters ();
}

void
Decoder::setup_audio ()
{
	_audio_codec_context = _format_context->streams[_audio_stream]->codec;
	_audio_codec = avcodec_find_decoder (_audio_codec_context->codec_id);

	if (_audio_codec == 0) {
		throw runtime_error ("Could not find audio decoder");
	}
	
	if (avcodec_open2 (_audio_codec_context, _audio_codec, 0) < 0) {
		throw runtime_error ("Could not open audio decoder");
	}
}

Decoder::PassResult
Decoder::pass ()
{
	int r = av_read_frame (_format_context, &_packet);
	if (r < 0) {
		return PASS_DONE;
	}

	if (_par->num_frames != 0 && _video_frame >= _par->num_frames) {
		return PASS_DONE;
	}

	if (_packet.stream_index == _video_stream && _par->decode_video) {
		
		int frame_finished;
		if (avcodec_decode_video2 (_video_codec_context, _frame_in, &frame_finished, &_packet) < 0) {
			return PASS_ERROR;
		}
		
		if (frame_finished) {

			++_video_frame;
			
			if (_par->decode_video_frequency != 0 && (_video_frame % (length_in_frames() / _par->decode_video_frequency)) != 0) {
				return PASS_NOTHING;
			}

			if (av_vsrc_buffer_add_frame (_buffer_src_context, _frame_in, 0) < 0) {
				throw runtime_error ("Could not push buffer into filter chain.");
			}
			
			while (avfilter_poll_frame (_buffer_sink_context->inputs[0])) {
				AVFilterBufferRef* filter_buffer;
				av_buffersink_get_buffer_ref (_buffer_sink_context, &filter_buffer, 0);
				if (filter_buffer) {

					uint8_t** p = filter_buffer->data;
					int* s = filter_buffer->linesize;

					if (_pp_mode) {
						/* Do FFMPEG post-processing */
						pp_postprocess (
							(const uint8_t **) filter_buffer->data, filter_buffer->linesize,
							_pp_buffer, _pp_stride,
							_post_filter_width, _post_filter_height,
							0, 0, _pp_mode, _pp_context, 0
							);

						p = _pp_buffer;
						s = _pp_stride;
					}

					/* Scale and convert from YUV to RGB */
					sws_scale (
						_conversion_context,
						p, s,
						0, filter_buffer->video->h,
						_frame_out->data, _frame_out->linesize
						);

					/* Emit the result */
					Video (_frame_out->data[0], _frame_out->linesize[0], _video_frame);
					
					avfilter_unref_buffer (filter_buffer);
					return PASS_VIDEO;
				}
			}
		}
		
	} else if (_packet.stream_index == _audio_stream && _par->decode_audio) {
		
		avcodec_get_frame_defaults (_frame_in);
		
		int frame_finished;
		if (avcodec_decode_audio4 (_audio_codec_context, _frame_in, &frame_finished, &_packet) < 0) {
			return PASS_ERROR;
		}
		
		if (frame_finished) {
			int const data_size = av_samples_get_buffer_size (
				0, _audio_codec_context->channels, _frame_in->nb_samples, audio_sample_format (), 1
				);
			
			Audio (_frame_in->data[0], _audio_codec_context->channels, data_size);

			return PASS_AUDIO;
		}
	}
	
	av_free_packet (&_packet);

	return PASS_NOTHING;
}

void
Decoder::setup_video_filters ()
{
	int r;
	
	string filters = Filter::ffmpeg_strings (_par->filters).first;

	stringstream fs;
	if (_par->apply_crop) {
		fs << "crop=" << _post_filter_width << ":" << _post_filter_height << ":" << _par->left_crop << ":" << _par->top_crop << " ";
	} else {
		fs << "crop=" << _post_filter_width << ":" << _post_filter_height << ":0:0";
	}
	
	if (!filters.empty ()) {
		filters += ",";
	}

	filters += fs.str ();

	avfilter_register_all ();
	
	AVFilterGraph* graph = avfilter_graph_alloc();
	if (graph == 0) {
		throw runtime_error ("Could not create filter graph.");
	}
	
	AVFilter* buffer_src = avfilter_get_by_name("buffer");
	if (buffer_src == 0) {
		throw runtime_error ("Could not create buffer src filter");
	}
	
	AVFilter* buffer_sink = avfilter_get_by_name("buffersink");
	if (buffer_sink == 0) {
		throw runtime_error ("Could not create buffer sink filter");
	}
	
	stringstream a;
	a << _video_codec_context->width << ":"
	  << _video_codec_context->height << ":"
	  << _video_codec_context->pix_fmt << ":"
	  << _video_codec_context->time_base.num << ":"
	  << _video_codec_context->time_base.den << ":"
	  << _video_codec_context->sample_aspect_ratio.num << ":"
	  << _video_codec_context->sample_aspect_ratio.den;

	if ((r = avfilter_graph_create_filter (&_buffer_src_context, buffer_src, "in", a.str().c_str(), 0, graph)) < 0) {
		stringstream s;
		s << "Could not create buffer source (" << r << ")";
		throw runtime_error (s.str().c_str ());
	}

	enum PixelFormat pixel_formats[] = { _video_codec_context->pix_fmt, PIX_FMT_NONE };
	if (avfilter_graph_create_filter(&_buffer_sink_context, buffer_sink, "out", 0, pixel_formats, graph) < 0) {
		throw runtime_error ("Could not create buffer sink.");
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

	if (avfilter_graph_parse (graph, filters.c_str(), &inputs, &outputs, 0) < 0) {
		throw runtime_error ("Could not set up filter graph.");
	}

	if (avfilter_graph_config (graph, 0) < 0) {
		throw runtime_error ("Could not configure filter graph.");
	}
}

int
Decoder::length_in_frames () const
{
	return (_format_context->duration / AV_TIME_BASE) * frames_per_second ();
}

float
Decoder::frames_per_second () const
{
	return av_q2d (_format_context->streams[_video_stream]->avg_frame_rate);
}

int
Decoder::audio_channels () const
{
	if (_audio_codec_context == 0) {
		return 0;
	}
	
	return _audio_codec_context->channels;
}

int
Decoder::audio_sample_rate () const
{
	if (_audio_codec_context == 0) {
		return 0;
	}
	
	return _audio_codec_context->sample_rate;
}

AVSampleFormat
Decoder::audio_sample_format () const
{
	return _audio_codec_context->sample_fmt;
}

int
Decoder::native_width () const
{
	return _video_codec_context->width;
}

int
Decoder::native_height () const
{
	return _video_codec_context->height;
}

void
Decoder::go ()
{
	while (pass () != PASS_DONE) {
		if (_job) {
			_job->set_progress (float (_video_frame) / decoding_frames ());
		}
	}
}

void
Decoder::setup_post_process_filters ()
{
	pair<string, string> s = Filter::ffmpeg_strings (_par->filters);
	if (s.second.empty ()) {
		return;
	}

	_pp_mode = pp_get_mode_by_name_and_quality (s.second.c_str(), PP_QUALITY_MAX);
	_pp_context = pp_get_context (_post_filter_width, _post_filter_height, PP_FORMAT_420 | PP_CPU_CAPS_MMX2);

	_pp_buffer = new uint8_t*[4];
	for (int i = 0; i < 4; ++i) {
		_pp_buffer[i] = new uint8_t[_post_filter_width * _post_filter_height];
		_pp_stride[i] = _post_filter_width;
	}
}

/** @return Number of frames that we will be decoding */
int
Decoder::decoding_frames () const
{
	if (_par->num_frames > 0) {
		return _par->num_frames;
	}
	
	return length_in_frames ();
}
