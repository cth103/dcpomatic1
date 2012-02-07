#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include "lut.h"

using namespace std;

char const video[] = "/home/carl/Films/Ghostbusters/DVD_VIDEO/VIDEO_TS/VTS_02_1.VOB";
int const nframes = 25 * 60;

/* TODO: cropping */

/* from opendcp */

/* Color Matrices */
static float color_matrix[3][3][3] = {
    /* SRGB */
    {{0.4124564, 0.3575761, 0.1804375},
     {0.2126729, 0.7151522, 0.0721750},
     {0.0193339, 0.1191920, 0.9503041}},

    /* REC.709 */
    {{0.4124564, 0.3575761, 0.1804375},
     {0.2126729, 0.7151522, 0.0721750},
     {0.0193339, 0.1191920, 0.9503041}},

    /* DC28.30 (2006-02-24) */
    {{0.4451698156, 0.2771344092, 0.1722826698},
     {0.2094916779, 0.7215952542, 0.0689130679},
     {0.0000000000, 0.0470605601, 0.9073553944}}
};

/* complex gamma function */
float complex_gamma(float p, float gamma) {
    float v;

    if ( p > 0.04045) {
        v = pow((p+0.055)/1.055,gamma);
    } else {
        v = p/12.92;
    }

    return v;
}

#define DCI_GAMMA      (2.6)
#define DCI_DEGAMMA    (1/DCI_GAMMA)
#define DCI_COEFFICENT (48.0/52.37)

void
rgb_to_xyz (uint8_t* data, int size)
{
	int const index = 0; /* for sRGB; could use 1 for REC 709 or 2 for DCI */

	uint8_t* p = data;

	cout << "RGB -> XYZ for " << size << "\n";
	
	for (int i = 0; i < size; i++) {

		/* in gamma lut */
		float sr = lut_in[index][p[0]];
		float sg = lut_in[index][p[1]];
		float sb = lut_in[index][p[2]];

		/* RGB to XYZ Matrix */
		float dx = ((sr * color_matrix[index][0][0]) + (sg * color_matrix[index][0][1]) + (sb * color_matrix[index][0][2]));
		float dy = ((sr * color_matrix[index][1][0]) + (sg * color_matrix[index][1][1]) + (sb * color_matrix[index][1][2]));
		float dz = ((sr * color_matrix[index][2][0]) + (sg * color_matrix[index][2][1]) + (sb * color_matrix[index][2][2]));

		/* DCI Companding */
		dx = dx * DCI_COEFFICENT * (DCI_LUT_SIZE-1);
		dy = dy * DCI_COEFFICENT * (DCI_LUT_SIZE-1);
		dz = dz * DCI_COEFFICENT * (DCI_LUT_SIZE-1);
	
		/* out gamma lut */
		p[0] = lut_out[0][(int)dx] >> 4;
		p[1] = lut_out[0][(int)dy] >> 4;
		p[2] = lut_out[0][(int)dz] >> 4;
		
		p += 3;
	}
}
	    
int
main (int argc, char* argv[])
{
	av_register_all ();

	AVFormatContext* format_context = NULL;
	if (avformat_open_input (&format_context, video, NULL, NULL) != 0) {
		fprintf (stderr, "avformat_open_input failed.\n");
		return -1;
	}

	if (avformat_find_stream_info (format_context, NULL) < 0) {
		fprintf (stderr, "av_find_stream_info failed.\n");
		return -1;
	}

	av_dump_format (format_context, 0, video, 0);

	int video_stream = -1;
	for (uint32_t i = 0; i < format_context->nb_streams; ++i) {
		if (format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream = i;
			break;
		}
	}

	AVCodecContext* decoder_context = format_context->streams[video_stream]->codec;

	AVCodec* decoder = avcodec_find_decoder (decoder_context->codec_id);
	if (decoder == NULL) {
		fprintf (stderr, "avcodec_find_decoder failed.\n");
		return -1;
	}

	if (avcodec_open2 (decoder_context, decoder, NULL) < 0) {
		fprintf (stderr, "avcodec_open failed.\n");
		return -1;
	}

	/* XXX */
	if (decoder_context->time_base.num > 1000 && decoder_context->time_base.den == 1) {
		decoder_context->time_base.den = 1000;
	}

	AVCodec* encoder = avcodec_find_encoder (CODEC_ID_JPEG2000);
	if (encoder == NULL) {
		cerr << "avcodec_find_encoder failed.\n";
		return -1;
	}

	AVCodecContext* encoder_context = avcodec_alloc_context3 (encoder);

	cout << decoder_context->width << " x " << decoder_context->height << "\n";
	encoder_context->width = decoder_context->width;
	encoder_context->height = decoder_context->height;
	/* XXX */
	encoder_context->time_base = (AVRational) {1, 25};
	encoder_context->pix_fmt = PIX_FMT_RGB24;//YUV420P;
	encoder_context->compression_level = 0;

	if (avcodec_open2 (encoder_context, encoder, NULL) < 0) {
		cerr << "avcodec_open failed.\n";
		return -1;
	}
	
	AVFrame* frame = avcodec_alloc_frame ();

	AVFrame* frame_RGB = avcodec_alloc_frame ();
	if (frame_RGB == NULL) {
		fprintf (stderr, "avcodec_alloc_frame failed.\n");
		return -1;
	}

	FILE* outfile = fopen ("output", "wb");

	int num_bytes = avpicture_get_size (PIX_FMT_RGB24, decoder_context->width, decoder_context->height);
	uint8_t* buffer = (uint8_t *) malloc (num_bytes);

	avpicture_fill ((AVPicture *) frame_RGB, buffer, PIX_FMT_RGB24, decoder_context->width, decoder_context->height);

	/* alloc image and output buffer */
	int outbuf_size = 1000000;
	uint8_t* outbuf = (uint8_t *) malloc (outbuf_size);
	int out_size = 0;
	
	AVPacket packet;
	int i = 0;
	while (av_read_frame (format_context, &packet) >= 0) {

		int frame_finished;

		if (packet.stream_index == video_stream) {
			avcodec_decode_video2 (decoder_context, frame, &frame_finished, &packet);

			if (frame_finished) {
				static struct SwsContext *img_convert_context;

				if (img_convert_context == NULL) {
					int w = decoder_context->width;
					int h = decoder_context->height;
					
					img_convert_context = sws_getContext (
						w, h, 
						decoder_context->pix_fmt, 
						w, h, PIX_FMT_RGB24, SWS_BICUBIC,
						NULL, NULL, NULL
						);
					
					if (img_convert_context == NULL) {
						fprintf (stderr, "sws_getContext failed.\n");
						return -1;
					}
				}
				
				sws_scale (
					img_convert_context, frame->data, frame->linesize, 0, 
					decoder_context->height, frame_RGB->data, frame_RGB->linesize
					);

				++i;
				printf("%d\n", i);
				if (i == 200) {
					printf ("%p %p %p\n", frame_RGB->data[0], frame_RGB->data[1], frame_RGB->data[2]);
					printf ("linesize %d\n", frame_RGB->linesize[0]);
					rgb_to_xyz (frame_RGB->data[0], encoder_context->width * encoder_context->height);
					out_size = avcodec_encode_video (encoder_context, outbuf, outbuf_size, frame_RGB);
					fwrite (outbuf, 1, out_size, outfile);
					fclose (outfile);
					return 0;
				}
			}
		}

		av_free_packet (&packet);
	}

	while (out_size) {
		out_size = avcodec_encode_video (encoder_context, outbuf, outbuf_size, NULL);
		fwrite (outbuf, 1, out_size, outfile);
	}

	fclose (outfile);
	free (buffer);
	av_free (frame_RGB);
	av_free (frame);
	avcodec_close (decoder_context);
	avcodec_close (encoder_context);
	avformat_close_input (&format_context);
	
	return 0;	
}
