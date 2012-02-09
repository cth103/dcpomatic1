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

Decoder::Decoder (Film const * f, int w, int h)
	: _film (f)
	, _out_width (w)
	, _out_height (h)
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
	, _decode_video (true)
	, _decode_video_period (1)
	, _video_frame (0)
	, _decode_audio (true)
	, _apply_crop (true)
{
	if (_film->format() == 0) {
		throw runtime_error ("Film format unknown");
	}

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
}	

void
Decoder::setup_general ()
{
	int r;
	
	av_register_all ();

	if ((r = avformat_open_input (&_format_context, _film->content().c_str(), 0, 0)) != 0) {
		stringstream s;
		s << "Could not open content file " << _film->content() << " (" << r << ")";
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

	int num_bytes = avpicture_get_size (PIX_FMT_RGB24, _out_width, _out_height);
	_frame_out_buffer = (uint8_t *) av_malloc (num_bytes);

	avpicture_fill ((AVPicture *) _frame_out, _frame_out_buffer, PIX_FMT_RGB24, _out_width, _out_height);

	int post_filter_width = _video_codec_context->width;
	int post_filter_height = _video_codec_context->height;
	
	if (_apply_crop) {
		post_filter_width -= _film->left_crop() - _film->right_crop();
		post_filter_height -= _film->top_crop() - _film->bottom_crop();		
	}

	_conversion_context = sws_getContext (
		post_filter_width, post_filter_height, _video_codec_context->pix_fmt,
		_out_width, _out_height, PIX_FMT_RGB24,
		SWS_BICUBIC, 0, 0, 0
		);

	if (_conversion_context == 0) {
		throw runtime_error ("Could not obtain YUV -> RGB conversion context");
	}

	stringstream fs;
	if (_apply_crop) {
		fs << "crop=" << post_filter_width << ":" << post_filter_height << ":" << _film->left_crop() << ":" << _film->top_crop() << " ";
	}
	setup_video_filters (fs.str());
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

	if (_packet.stream_index == _video_stream && _decode_video) {
		
		int frame_finished;
		if (avcodec_decode_video2 (_video_codec_context, _frame_in, &frame_finished, &_packet) < 0) {
			return PASS_ERROR;
		}
		
		if (frame_finished) {

			++_video_frame;
			
			if (_decode_video_period != 1 && (_video_frame % _decode_video_period) != 0) {
				return PASS_NOTHING;
			}

			if (av_vsrc_buffer_add_frame (_buffer_src_context, _frame_in, 0) < 0) {
				throw runtime_error ("Could not push buffer into filter chain.");
			}
			
			while (avfilter_poll_frame (_buffer_sink_context->inputs[0])) {
				AVFilterBufferRef* filter_buffer;
				av_buffersink_get_buffer_ref (_buffer_sink_context, &filter_buffer, 0);
				if (filter_buffer) {
					
					/* Scale and convert from YUV to RGB */
					sws_scale (
						_conversion_context,
						filter_buffer->data, filter_buffer->linesize,
						0, filter_buffer->video->h,
						_frame_out->data, _frame_out->linesize
						);
					
					process_video (_frame_out->data[0], _frame_out->linesize[0]);
					
					avfilter_unref_buffer (filter_buffer);
					return PASS_VIDEO;
				}
			}
		}
		
	} else if (_packet.stream_index == _audio_stream && _decode_audio) {
		
		avcodec_get_frame_defaults (_frame_in);
		
		int frame_finished;
		if (avcodec_decode_audio4 (_audio_codec_context, _frame_in, &frame_finished, &_packet) < 0) {
			return PASS_ERROR;
		}
		
		if (frame_finished) {
			int const data_size = av_samples_get_buffer_size (
				0, _audio_codec_context->channels, _frame_in->nb_samples, audio_sample_format (), 1
				);
			
			process_audio (_frame_in->data[0], _audio_codec_context->channels, data_size);

			return PASS_AUDIO;
		}
	}
	
	av_free_packet (&_packet);

	return PASS_NOTHING;
}

void
Decoder::setup_video_filters (string const & filters)
{
	int r;

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


void
Decoder::decode_video (bool w)
{
	_decode_video = w;
}

void
Decoder::decode_audio (bool w)
{
	_decode_audio = w;
}

void
Decoder::set_decode_video_period (int p)
{
	_decode_video_period = p;
}

void
Decoder::apply_crop (bool c)
{
	_apply_crop = c;
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

	
       
