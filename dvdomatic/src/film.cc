#include <stdexcept>
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
#include "film.h"

using namespace std;

Film::Film (string const & c)
	: _content (c)
	, _left_crop (0)
	, _right_crop (0)
	, _top_crop (0)
	, _bottom_crop (0)
{

}

void
Film::make_tiffs (string const & dir, int N)
{
	av_register_all();
    
	AVFormatContext* format_context = 0;
	if (avformat_open_input (&format_context, _content.c_str(), 0, 0) != 0) {
		throw runtime_error ("Could not open content file");
	}

	if (avformat_find_stream_info (format_context, 0) < 0) {
		throw runtime_error ("Could not find stream information");
	}

	int video_stream = -1;
	for (uint32_t i = 0; i < format_context->nb_streams; ++i) {
		if (format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream = i;
			break;
		}
	}

	if (video_stream < 0) {
		throw runtime_error ("Could not find video stream");
	}

	AVCodecContext* codec_context = format_context->streams[video_stream]->codec;
	AVCodec* codec = avcodec_find_decoder (codec_context->codec_id);
	if (codec == 0) {
		throw runtime_error ("Could not find decoder");
	}

	if (avcodec_open2 (codec_context, codec, 0) < 0) {
		throw runtime_error ("Could not open decoder");
	}

	AVFrame* frame_in = avcodec_alloc_frame ();
	if (frame_in == 0) {
		throw runtime_error ("Could not allocate frame");
	}
	
	AVFrame* frame_out = avcodec_alloc_frame ();
	if (frame_out == 0) {
		throw runtime_error ("Could not allocate frame");
	}

	int shit = 1000;
	int piss = 500;

	int num_bytes = avpicture_get_size (PIX_FMT_RGB24, shit, piss);
	uint8_t* frame_out_buffer = new uint8_t[num_bytes];

	avpicture_fill ((AVPicture *) frame_out, frame_out_buffer, PIX_FMT_RGB24, shit, piss);

	int const post_filter_width = codec_context->width - _left_crop - _right_crop;
	int const post_filter_height = codec_context->height - _top_crop - _bottom_crop;

	struct SwsContext* conversion_context = sws_getContext (
		post_filter_width, post_filter_height, codec_context->pix_fmt,
		shit, piss, PIX_FMT_RGB24,
		SWS_BICUBIC, 0, 0, 0
		);

	if (conversion_context == 0) {
		throw runtime_error ("Could not obtain YUV -> RGB conversion context");
	}

	int frame = 0;
	AVPacket packet;

	stringstream fs;
	fs << "crop=" << post_filter_width << ":" << post_filter_height << ":" << _left_crop << ":" << _top_crop << " ";

	pair<AVFilterContext *, AVFilterContext *> filters = setup_filters (codec_context, fs.str());
	while (av_read_frame (format_context, &packet) >= 0 && (N == 0 || frame < N)) {

		if (packet.stream_index == video_stream) {
			int frame_finished;
			if (avcodec_decode_video2 (codec_context, frame_in, &frame_finished, &packet) < 0) {
				throw runtime_error ("Error decoding video");
			}
			av_free_packet (&packet);
			
			if (frame_finished) {

				if (av_vsrc_buffer_add_frame (filters.first, frame_in, 0) < 0) {
					throw runtime_error ("Could not push buffer into filter chain.");
				}
				
				while (avfilter_poll_frame (filters.second->inputs[0])) {
					AVFilterBufferRef* filter_buffer;
					av_buffersink_get_buffer_ref (filters.second, &filter_buffer, 0);
					if (filter_buffer) {

						/* Scale and convert from YUV to RGB */
						sws_scale (
							conversion_context,
							filter_buffer->data, filter_buffer->linesize,
							0, filter_buffer->video->h,
							frame_out->data, frame_out->linesize
							);

						++frame;
						write_tiff (dir, frame, frame_out->data[0], shit, piss);
					}
				}
			}
		}
	}
	
	delete[] frame_out_buffer;
	av_free (frame_in);
	av_free (frame_out);
	avcodec_close (codec_context);
	avformat_close_input (&format_context);
}

void
Film::write_tiff (string const & dir, int frame, uint8_t* data, int w, int h) const
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
	TIFFSetField (output, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
	TIFFSetField (output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField (output, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	TIFFSetField (output, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField (output, TIFFTAG_SAMPLESPERPIXEL, 3);
	
	if (TIFFWriteEncodedStrip (output, 0, data, w * h * 3) == 0) {
		throw runtime_error ("Failed to write to output TIFF file");
	}

	TIFFClose (output);
}

std::pair<AVFilterContext*, AVFilterContext*>
Film::setup_filters (AVCodecContext* codec_context, string const & filters)
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
	a << codec_context->width << ":"
	  << codec_context->height << ":"
	  << codec_context->pix_fmt << ":"
	  << codec_context->time_base.num << ":"
	  << codec_context->time_base.den << ":"
	  << codec_context->sample_aspect_ratio.num << ":"
	  << codec_context->sample_aspect_ratio.den;

	AVFilterContext* buffer_src_context = 0;
	if ((r = avfilter_graph_create_filter (&buffer_src_context, buffer_src, "in", a.str().c_str(), 0, graph)) < 0) {
		stringstream s;
		s << "Could not create buffer source (" << r << ")";
		throw runtime_error (s.str().c_str ());
	}

	enum PixelFormat pixel_formats[] = { codec_context->pix_fmt, PIX_FMT_NONE };
	AVFilterContext* buffer_sink_context = 0;
	if (avfilter_graph_create_filter(&buffer_sink_context, buffer_sink, "out", 0, pixel_formats, graph) < 0) {
		throw runtime_error ("Could not create buffer sink.");
	}

	AVFilterInOut* outputs = avfilter_inout_alloc ();
	outputs->name = av_strdup("in");
	outputs->filter_ctx = buffer_src_context;
	outputs->pad_idx = 0;
	outputs->next = 0;

	AVFilterInOut* inputs = avfilter_inout_alloc ();
	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffer_sink_context;
	inputs->pad_idx = 0;
	inputs->next = 0;

	if (avfilter_graph_parse (graph, filters.c_str(), &inputs, &outputs, 0) < 0) {
		throw runtime_error ("Could not set up filter graph.");
	}

	if (avfilter_graph_config (graph, 0) < 0) {
		throw runtime_error ("Could not configure filter graph.");
	}

	return make_pair (buffer_src_context, buffer_sink_context);
}

void
Film::set_top_crop (int c)
{
	_top_crop = c;
}

void
Film::set_bottom_crop (int c)
{
	_bottom_crop = c;
}

void
Film::set_left_crop (int c)
{
	_left_crop = c;
}

void
Film::set_right_crop (int c)
{
	_right_crop = c;
}

