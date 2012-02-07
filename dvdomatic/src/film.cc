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
#include "film.h"
#include "format.h"

using namespace std;

Film::Film (string const & c)
	: _content (c)
	, _format (0)
	, _left_crop (0)
	, _right_crop (0)
	, _top_crop (0)
	, _bottom_crop (0)
{

}

void
Film::make_tiffs_and_wavs (string const & tiffs, string const & wavs, int N)
{
	if (_format == 0) {
		throw runtime_error ("Film format unknown");
	}
	
	av_register_all();

	AVFormatContext* format_context = 0;
	if (avformat_open_input (&format_context, _content.c_str(), 0, 0) != 0) {
		throw runtime_error ("Could not open content file");
	}

	if (avformat_find_stream_info (format_context, 0) < 0) {
		throw runtime_error ("Could not find stream information");
	}

	int video_stream = -1;
	int audio_stream = -1;
	for (uint32_t i = 0; i < format_context->nb_streams; ++i) {
		if (format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream = i;
		} else if (format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream = i;
		}
	}

	if (video_stream < 0) {
		throw runtime_error ("Could not find video stream");
	}
	if (audio_stream < 0) {
		throw runtime_error ("Could not find audio stream");
	}
	
	AVCodecContext* video_codec_context = format_context->streams[video_stream]->codec;
	AVCodec* video_codec = avcodec_find_decoder (video_codec_context->codec_id);
	if (video_codec == 0) {
		throw runtime_error ("Could not find video decoder");
	}
	if (avcodec_open2 (video_codec_context, video_codec, 0) < 0) {
		throw runtime_error ("Could not open video decoder");
	}

	AVCodecContext* audio_codec_context = format_context->streams[audio_stream]->codec;
	AVCodec* audio_codec = avcodec_find_decoder (audio_codec_context->codec_id);
	if (audio_codec == 0) {
		throw runtime_error ("Could not find audio decoder");
	}
	if (avcodec_open2 (audio_codec_context, audio_codec, 0) < 0) {
		throw runtime_error ("Could not open audio decoder");
	}

	/* Create sound output files */
	vector<SNDFILE*> sound_files;
	for (int i = 0; i < audio_codec_context->channels; ++i) {
		stringstream wav_path;
		wav_path << wavs << "/" << (i + 1) << ".wav";
		SF_INFO sf_info;
		sf_info.samplerate = audio_codec_context->sample_rate;
		/* We write mono files */
		sf_info.channels = 1;
		sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
		SNDFILE* f = sf_open (wav_path.str().c_str(), SFM_WRITE, &sf_info);
		if (f == 0) {
			throw runtime_error ("Could not create audio output file");
		}
		sound_files.push_back (f);
	}

	/* Create buffer for deinterleaving audio */
	int const deinterleave_buffer_size = 8192;
	uint8_t* deinterleave_buffer = new uint8_t[deinterleave_buffer_size];

	AVFrame* frame_in = avcodec_alloc_frame ();
	if (frame_in == 0) {
		throw runtime_error ("Could not allocate frame");
	}
	
	AVFrame* frame_out = avcodec_alloc_frame ();
	if (frame_out == 0) {
		throw runtime_error ("Could not allocate frame");
	}

	int num_bytes = avpicture_get_size (PIX_FMT_RGB24, _format->dci_width (), _format->dci_height ());
	uint8_t* frame_out_buffer = new uint8_t[num_bytes];

	avpicture_fill ((AVPicture *) frame_out, frame_out_buffer, PIX_FMT_RGB24, _format->dci_width(), _format->dci_height ());

	int const post_filter_width = video_codec_context->width - _left_crop - _right_crop;
	int const post_filter_height = video_codec_context->height - _top_crop - _bottom_crop;

	struct SwsContext* conversion_context = sws_getContext (
		post_filter_width, post_filter_height, video_codec_context->pix_fmt,
		_format->dci_width(), _format->dci_height(), PIX_FMT_RGB24,
		SWS_BICUBIC, 0, 0, 0
		);

	if (conversion_context == 0) {
		throw runtime_error ("Could not obtain YUV -> RGB conversion context");
	}

	int frame = 0;
	AVPacket packet;

	stringstream fs;
	fs << "crop=" << post_filter_width << ":" << post_filter_height << ":" << _left_crop << ":" << _top_crop << " ";

	pair<AVFilterContext *, AVFilterContext *> filters = setup_filters (video_codec_context, fs.str());
	while (av_read_frame (format_context, &packet) >= 0 && (N == 0 || frame < N)) {

		if (packet.stream_index == video_stream) {

			/* VIDEO */
			
			int frame_finished;
			if (avcodec_decode_video2 (video_codec_context, frame_in, &frame_finished, &packet) < 0) {
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
//						write_tiff (tiffs, frame, frame_out->data[0], _format->dci_width(), _format->dci_height());
					}
				}
			}

		} else if (packet.stream_index == audio_stream) {

			/* AUDIO */

			avcodec_get_frame_defaults (frame_in);

			int frame_finished;
			if (avcodec_decode_audio4 (audio_codec_context, frame_in, &frame_finished, &packet) < 0) {
				throw runtime_error ("Error decoding video");
			}
			av_free_packet (&packet);

			if (frame_finished) {

				write_wav (audio_codec_context, frame_in);

				int const channels = audio_codec_context->channels;

				int const data_size = av_samples_get_buffer_size (
					0, channels, frame_in->nb_samples, audio_codec_context->sample_fmt, 1
					);

				/* Size of a sample in bytes */
				int const sample_size = 2;

				/* Number of samples left to read this time */
				int remaining_samples = data_size / (channels * sample_size);
				/* Our position in the output buffers, in samples */
				int position = 0;
				while (remaining_samples > 0) {
					/* How many samples to do this time; we can fill a buffer with single-sample frames */
					int this_time = min (remaining_samples, deinterleave_buffer_size / sample_size);
					for (int i = 0; i < channels; ++i) {
						for (int j = 0; j < this_time; ++j) {
							for (int k = 0; k < sample_size; ++k) {
								deinterleave_buffer[j * sample_size] = frame_in->data[0][position + (j + i) * sample_size + k];
							}
						}

						switch (audio_codec_context->sample_fmt) {
						case AV_SAMPLE_FMT_S16:
							sf_write_int (sound_files[i], (const int *) deinterleave_buffer, this_time);
							break;
						default:
							throw runtime_error ("Unknown audio sample format");
						}
					}

					position += this_time;
					remaining_samples -= this_time;
				}
			}
		}
	}
	
	delete[] frame_out_buffer;
	delete[] deinterleave_buffer;
	av_free (frame_in);
	av_free (frame_out);
	avcodec_close (video_codec_context);
	avformat_close_input (&format_context);
	for (vector<SNDFILE*>::iterator i = sound_files.begin(); i != sound_files.end(); ++i) {
		sf_close (*i);
	}
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

void
Film::write_wav (AVContext* codec_context, AVFrame* frame)
{
	int const channels = codec_context->channels;
	int const data_size = av_samples_get_buffer_size (
		0, channels, frame->nb_samples, codec_context->sample_fmt, 1
		);

	if (
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

void
Film::set_format (Format* f)
{
	_format = f;
}
