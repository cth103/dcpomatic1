#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include "opendcp.h"
#include "opendcp_image.h"
#include "openjpeg.h"

using namespace std;

char const video[] = "/home/carl/Films/Ghostbusters/DVD_VIDEO/VIDEO_TS/VTS_02_1.VOB";
int const nframes = 25 * 60;

/* TODO: cropping */

/* convert opendcp to openjpeg image format */
int odcp_to_opj(odcp_image_t *odcp, opj_image_t **opj_ptr) {
    OPJ_COLOR_SPACE color_space;
    opj_image_cmptparm_t cmptparm[3];
    opj_image_t *opj = NULL;
    int j,size;

    color_space = CLRSPC_SRGB;

    /* initialize image components */
    memset(&cmptparm[0], 0, odcp->n_components * sizeof(opj_image_cmptparm_t));
    for (j = 0;j <  odcp->n_components;j++) {
            cmptparm[j].w = odcp->w;
            cmptparm[j].h = odcp->h;
            cmptparm[j].prec = odcp->precision;
            cmptparm[j].bpp = odcp->bpp;
            cmptparm[j].sgnd = odcp->signed_bit;
            cmptparm[j].dx = odcp->dx;
            cmptparm[j].dy = odcp->dy;
    }

    /* create the image */
    opj = opj_image_create(odcp->n_components, &cmptparm[0], color_space);

    if(!opj) {
        dcp_log(LOG_ERROR,"Failed to create image");
        return DCP_FATAL;
    }

    /* set image offset and reference grid */
    opj->x0 = odcp->x0;
    opj->y0 = odcp->y0;
    opj->x1 = odcp->x1; 
    opj->y1 = odcp->y1; 

    size = odcp->w * odcp->h;

    memcpy(opj->comps[0].data,odcp->component[0].data,size*sizeof(int));
    memcpy(opj->comps[1].data,odcp->component[1].data,size*sizeof(int));
    memcpy(opj->comps[2].data,odcp->component[2].data,size*sizeof(int));

    *opj_ptr = opj;
    return DCP_SUCCESS;
}

int encode_openjpeg(opendcp_t *opendcp, opj_image_t *opj_image, char *out_file) {
    bool result;
    int codestream_length;
    int max_comp_size;
    int max_cs_len;
    opj_cparameters_t parameters;
    opj_cio_t *cio = NULL;
    opj_cinfo_t *cinfo = NULL;
    FILE *f = NULL; 
    int bw;
   
    if (opendcp->bw) {
        bw = opendcp->bw;
    } else {
        bw = MAX_DCP_JPEG_BITRATE;
    }

    /* set the max image and component sizes based on frame_rate */
    max_cs_len = ((float)bw)/8/opendcp->frame_rate;
 
    /* adjust cs for 3D */
    if (opendcp->stereoscopic) {
        max_cs_len = max_cs_len/2;
    } 
 
    max_comp_size = ((float)max_cs_len)/1.25;

    /* set encoding parameters to default values */
    opj_set_default_encoder_parameters(&parameters);

    /* set default cinema parameters */
    set_cinema_encoder_parameters(opendcp, &parameters);

    parameters.cp_comment = (char*)malloc(strlen(OPENDCP_NAME)+1);
    sprintf(parameters.cp_comment,"%s", OPENDCP_NAME);

    /* adjust cinema enum type */
    if (opendcp->cinema_profile == DCP_CINEMA4K) {
        parameters.cp_cinema = CINEMA4K_24;
    } else {
        parameters.cp_cinema = CINEMA2K_24;
    }

    /* Decide if MCT should be used */
    parameters.tcp_mct = opj_image->numcomps == 3 ? 1 : 0;

    /* set max image */
    parameters.max_comp_size = max_comp_size;
    parameters.tcp_rates[0]= ((float) (opj_image->numcomps * opj_image->comps[0].w * opj_image->comps[0].h * opj_image->comps[0].prec))/
                              (max_cs_len * 8 * opj_image->comps[0].dx * opj_image->comps[0].dy);

    /* get a J2K compressor handle */
    dcp_log(LOG_DEBUG,"%-15.15s: creating compressor %s","encode_openjpeg",out_file);
    cinfo = opj_create_compress(CODEC_J2K);

    /* set event manager to null (openjpeg 1.3 bug) */
    cinfo->event_mgr = NULL;

    /* setup the encoder parameters using the current image and user parameters */
    dcp_log(LOG_DEBUG,"%-15.15s: setup J2k encoder %s","encode_openjpeg",out_file);
    opj_setup_encoder(cinfo, &parameters, opj_image);

    /* open a byte stream for writing */
    /* allocate memory for all tiles */
    dcp_log(LOG_DEBUG,"%-15.15s: opening J2k output stream %s","encode_openjpeg",out_file);
    cio = opj_cio_open((opj_common_ptr)cinfo, NULL, 0);

    dcp_log(LOG_INFO,"Encoding file %s",out_file);
    result = opj_encode(cinfo, cio, opj_image, NULL);
    dcp_log(LOG_DEBUG,"%-15.15s: encoding file %s complete","encode_openjepg",out_file);

    if (!result) {
        dcp_log(LOG_ERROR,"Unable to encode jpeg2000 file %s",out_file);
        opj_cio_close(cio);
        opj_destroy_compress(cinfo);
        return DCP_FATAL;
    }
      
    codestream_length = cio_tell(cio);

    f = fopen(out_file, "wb");

    if (!f) {
        dcp_log(LOG_ERROR,"Unable to write jpeg2000 file %s",out_file);
        opj_cio_close(cio);
        opj_destroy_compress(cinfo);
        return DCP_FATAL;
    }

    fwrite(cio->buffer, 1, codestream_length, f);
    fclose(f);

    /* free openjpeg structure */
    opj_cio_close(cio);
    opj_destroy_compress(cinfo);

    /* free user parameters structure */
    if(parameters.cp_comment) free(parameters.cp_comment);
    if(parameters.cp_matrice) free(parameters.cp_matrice);

    return DCP_SUCCESS;
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
				if (i == 200) {

					opendcp_t opendcp;
					/* parameters used by encode_openjpeg */
					opendcp.cinema_profile = DCP_CINEMA2K;
					opendcp.frame_rate = 24;
					opendcp.stereoscopic = 0;
					opendcp.bw = 250;

					int const pixels = decoder_context->width * decoder_context->height;
					odcp_image_t odcp_image;
					odcp_image.component = new odcp_image_component_t[3];
					for (int i = 0; i < 3; ++i) {
						odcp_image.component[i].component_number = i;
						odcp_image.component[i].data = new int[pixels];
					}

					uint8_t* p = frame_RGB->data[0];
					for (int i = 0; i < pixels; ++i) {
						odcp_image.component[0].data[i] = p[0] << 4;
						odcp_image.component[1].data[i] = p[1] << 4;
						odcp_image.component[2].data[i] = p[2] << 4;
						p += 3;
					}

					/* parameters are LUT, xyz method */
					rgb_to_xyz (&odcp_image, 0, 0);

					/* XXX: don't do this, go straight to opj */
					

					/* -> libopenjpeg */
					opj_image_t* opj_image;
					odcp_to_opj (&odcp_image, opj_image);

					/* -> j2k */
					encode_openjpeg (&opendcp, opj_image, "frobozz.j2c");
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
