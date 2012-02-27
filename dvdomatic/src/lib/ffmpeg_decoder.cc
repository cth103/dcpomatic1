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

/** @file  src/ffmpeg_decoder.cc
 *  @brief A decoder using FFmpeg to decode content.
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
#include "film_state.h"
#include "options.h"
#include "exceptions.h"
#include "image.h"
#include "util.h"
#include "log.h"
#include "ffmpeg_decoder.h"

using namespace std;
using namespace boost;

FFmpegDecoder::FFmpegDecoder (boost::shared_ptr<const FilmState> s, boost::shared_ptr<const Options> o, Job* j, Log* l, bool minimal, bool ignore_length)
	: Decoder (s, o, j, l, minimal, ignore_length)
	, _format_context (0)
	, _video_stream (-1)
	, _audio_stream (-1)
	, _frame_in (0)
	, _video_codec_context (0)
	, _video_codec (0)
	, _buffer_src_context (0)
	, _buffer_sink_context (0)
	, _audio_codec_context (0)
	, _audio_codec (0)
	, _post_filter_width (0)
	, _post_filter_height (0)
{
	setup_general ();
	setup_video ();
	setup_audio ();
}

FFmpegDecoder::~FFmpegDecoder ()
{
	if (_audio_codec_context) {
		avcodec_close (_audio_codec_context);
	}
	
	if (_video_codec_context) {
		avcodec_close (_video_codec_context);
	}
	
	av_free (_frame_in);
	avformat_close_input (&_format_context);
}	

void
FFmpegDecoder::setup_general ()
{
	int r;
	
	av_register_all ();

	if ((r = avformat_open_input (&_format_context, _fs->file (_fs->content).c_str(), 0, 0)) != 0) {
		throw OpenFileError (_fs->file (_fs->content));
	}

	if (avformat_find_stream_info (_format_context, 0) < 0) {
		throw DecodeError ("could not find stream information");
	}

	for (uint32_t i = 0; i < _format_context->nb_streams; ++i) {
		if (_format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			_video_stream = i;
		} else if (_format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			_audio_stream = i;
		}
	}

	if (_video_stream < 0) {
		throw DecodeError ("could not find video stream");
	}
	if (_audio_stream < 0) {
		throw DecodeError ("could not find audio stream");
	}

	_frame_in = avcodec_alloc_frame ();
	if (_frame_in == 0) {
		throw DecodeError ("could not allocate frame");
	}
}

void
FFmpegDecoder::setup_video ()
{
	_video_codec_context = _format_context->streams[_video_stream]->codec;
	_video_codec = avcodec_find_decoder (_video_codec_context->codec_id);

	if (_video_codec == 0) {
		throw DecodeError ("could not find video decoder");
	}
	
	if (avcodec_open2 (_video_codec_context, _video_codec, 0) < 0) {
		throw DecodeError ("could not open video decoder");
	}

	_post_filter_width = _video_codec_context->width;
	_post_filter_height = _video_codec_context->height;
	
	if (_opt->apply_crop) {
		_post_filter_width -= _fs->left_crop + _fs->right_crop;
		_post_filter_height -= _fs->top_crop + _fs->bottom_crop;		
	}

	setup_video_filters ();
}

void
FFmpegDecoder::setup_audio ()
{
	_audio_codec_context = _format_context->streams[_audio_stream]->codec;
	_audio_codec = avcodec_find_decoder (_audio_codec_context->codec_id);

	if (_audio_codec == 0) {
		throw DecodeError ("could not find audio decoder");
	}
	
	if (avcodec_open2 (_audio_codec_context, _audio_codec, 0) < 0) {
		throw DecodeError ("could not open audio decoder");
	}
}

FFmpegDecoder::PassResult
FFmpegDecoder::do_pass ()
{
	int r = av_read_frame (_format_context, &_packet);
	if (r < 0) {
		return PASS_DONE;
	}

	if (_packet.stream_index == _video_stream && _opt->decode_video) {
		
		int frame_finished;
		if (avcodec_decode_video2 (_video_codec_context, _frame_in, &frame_finished, &_packet) < 0) {
			av_free_packet (&_packet);
			return PASS_ERROR;
		}
		
		if (frame_finished) {

			if (!have_video_frame_ready ()) {
				av_free_packet (&_packet);
				return PASS_NOTHING;
			}

			if (av_vsrc_buffer_add_frame (_buffer_src_context, _frame_in, 0) < 0) {
				throw DecodeError ("could not push buffer into filter chain.");
			}

			while (avfilter_poll_frame (_buffer_sink_context->inputs[0])) {
				AVFilterBufferRef* filter_buffer;
				if (av_buffersink_get_buffer_ref (_buffer_sink_context, &filter_buffer, 0) >= 0) {

					/* This takes ownership of filter_buffer */
					shared_ptr<Image> image (new FilterBufferImage (_video_codec_context->pix_fmt, filter_buffer));

					emit_video (image);

					av_free_packet (&_packet);
					return PASS_VIDEO;
				}
			}
		}
		
	} else if (_packet.stream_index == _audio_stream && _opt->decode_audio) {
		
		avcodec_get_frame_defaults (_frame_in);
		
		int frame_finished;
		if (avcodec_decode_audio4 (_audio_codec_context, _frame_in, &frame_finished, &_packet) < 0) {
			av_free_packet (&_packet);
			return PASS_ERROR;
		}
		
		if (frame_finished) {
			int const data_size = av_samples_get_buffer_size (
				0, _audio_codec_context->channels, _frame_in->nb_samples, audio_sample_format (), 1
				);

			emit_audio (_frame_in->data[0], _audio_codec_context->channels, data_size);
			av_free_packet (&_packet);
			return PASS_AUDIO;
		}
	}
	
	av_free_packet (&_packet);
	return PASS_NOTHING;
}

void
FFmpegDecoder::setup_video_filters ()
{
	int r;
	
	string filters = Filter::ffmpeg_strings (_fs->filters).first;

	stringstream fs;
	if (_opt->apply_crop) {
		fs << "crop=" << _post_filter_width << ":" << _post_filter_height << ":" << _fs->left_crop << ":" << _fs->top_crop << " ";
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
	a << _video_codec_context->width << ":"
	  << _video_codec_context->height << ":"
	  << _video_codec_context->pix_fmt << ":"
	  << _video_codec_context->time_base.num << ":"
	  << _video_codec_context->time_base.den << ":"
	  << _video_codec_context->sample_aspect_ratio.num << ":"
	  << _video_codec_context->sample_aspect_ratio.den;

	if ((r = avfilter_graph_create_filter (&_buffer_src_context, buffer_src, "in", a.str().c_str(), 0, graph)) < 0) {
		throw DecodeError ("could not create buffer source");
	}

	enum PixelFormat pixel_formats[] = { _video_codec_context->pix_fmt, PIX_FMT_NONE };
	if (avfilter_graph_create_filter(&_buffer_sink_context, buffer_sink, "out", 0, pixel_formats, graph) < 0) {
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

int
FFmpegDecoder::length_in_frames () const
{
	return (_format_context->duration / AV_TIME_BASE) * frames_per_second ();
}

float
FFmpegDecoder::frames_per_second () const
{
	return av_q2d (_format_context->streams[_video_stream]->avg_frame_rate);
}

int
FFmpegDecoder::audio_channels () const
{
	if (_audio_codec_context == 0) {
		return 0;
	}
	
	return _audio_codec_context->channels;
}

int
FFmpegDecoder::audio_sample_rate () const
{
	if (_audio_codec_context == 0) {
		return 0;
	}
	
	return _audio_codec_context->sample_rate;
}

AVSampleFormat
FFmpegDecoder::audio_sample_format () const
{
	return _audio_codec_context->sample_fmt;
}

Size
FFmpegDecoder::native_size () const
{
	return Size (_video_codec_context->width, _video_codec_context->height);
}
