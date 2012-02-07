#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <iostream>
extern "C" {
#include <tiffio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include "film.h"

using namespace std;

Film::Film (string const & c)
	: _content (c)
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

	AVFrame* frame_yuv = avcodec_alloc_frame ();
	if (frame_yuv == 0) {
		throw runtime_error ("Could not allocate frame");
	}
	
	AVFrame* frame_rgb = avcodec_alloc_frame ();
	if (frame_rgb == 0) {
		throw runtime_error ("Could not allocate frame");
	}

	int num_bytes = avpicture_get_size (PIX_FMT_RGB24, codec_context->width, codec_context->height);
	uint8_t* frame_rgb_buffer = new uint8_t[num_bytes];

	avpicture_fill ((AVPicture *) frame_rgb, frame_rgb_buffer, PIX_FMT_RGB24, codec_context->width, codec_context->height);

	struct SwsContext* conversion_context = sws_getContext (
		codec_context->width, codec_context->height, codec_context->pix_fmt,
		codec_context->width, codec_context->height, PIX_FMT_RGB24,
		SWS_BICUBIC, 0, 0, 0
		);

	if (conversion_context == 0) {
		throw runtime_error ("Could not obtain YUV -> RGB conversion context");
	}

	int frame = 0;
	AVPacket packet;
	while (av_read_frame (format_context, &packet) >= 0 && (N == 0 || frame < N)) {

		if (packet.stream_index == video_stream) {
			int frame_finished;
			avcodec_decode_video2 (codec_context, frame_yuv, &frame_finished, &packet);
			if (frame_finished) {

				/* Convert from YUV to RGB */
				sws_scale (
					conversion_context,
					frame_yuv->data, frame_yuv->linesize,
					0, codec_context->height,
					frame_rgb->data, frame_rgb->linesize
					);

				/* Write TIFF */
				++frame;
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

				TIFFSetField (output, TIFFTAG_IMAGEWIDTH, codec_context->width);
				TIFFSetField (output, TIFFTAG_IMAGELENGTH, codec_context->height);
				TIFFSetField (output, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
				TIFFSetField (output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
				TIFFSetField (output, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
				TIFFSetField (output, TIFFTAG_BITSPERSAMPLE, 8);
				TIFFSetField (output, TIFFTAG_SAMPLESPERPIXEL, 3);

				if (TIFFWriteEncodedStrip (output, 0, frame_rgb->data[0], codec_context->width * codec_context->height * 3) == 0) {
					throw runtime_error ("Failed to write to output TIFF file");
				}

				TIFFClose (output);
			}
		}
	
		av_free_packet(&packet);
	}
	
	delete[] frame_rgb_buffer;
	av_free (frame_rgb);
	av_free (frame_yuv);
	avcodec_close (codec_context);
	avformat_close_input (&format_context);
}
