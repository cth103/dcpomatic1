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

using namespace std;

FilmWriter::FilmWriter (Film* f, string const & t, string const & w, int N)
	: _film (f)
	, _tiffs (t)
	, _wavs (w)
	, _nframes (N)
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
	, _deinterleave_buffer_size (8192)
	, _deinterleave_buffer (0)
	, _frame (0)
{
	if (_film->format() == 0) {
		throw runtime_error ("Film format unknown");
	}

	setup_general ();
	setup_video ();
	setup_audio ();
	decode ();
}

FilmWriter::~FilmWriter ()
{
	/* XXX: lots of stuff not dealloced in here */
	
	delete[] _deinterleave_buffer;

	for (vector<SNDFILE*>::iterator i = _sound_files.begin(); i != _sound_files.end(); ++i) {
		sf_close (*i);
	}

	if (_audio_codec_context) {
		avcodec_close (_audio_codec_context);
	}
	
	delete [] _frame_out_buffer;
	if (_video_codec_context) {
		avcodec_close (_video_codec_context);
	}
	av_free (_frame_out);
	av_free (_frame_in);
	avformat_close_input (&_format_context);
}	

void
FilmWriter::setup_general ()
{
	av_register_all ();

	if (avformat_open_input (&_format_context, _film->content().c_str(), 0, 0) != 0) {
		throw runtime_error ("Could not open content file");
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
FilmWriter::setup_video ()
{
	_video_codec_context = _format_context->streams[_video_stream]->codec;
	_video_codec = avcodec_find_decoder (_video_codec_context->codec_id);

	if (_video_codec == 0) {
		throw runtime_error ("Could not find video decoder");
	}
	
	if (avcodec_open2 (_video_codec_context, _video_codec, 0) < 0) {
		throw runtime_error ("Could not open video decoder");
	}
	
	int num_bytes = avpicture_get_size (PIX_FMT_RGB24, _film->format()->dci_width (), _film->format()->dci_height ());
	_frame_out_buffer = new uint8_t[num_bytes];

	avpicture_fill ((AVPicture *) _frame_out, _frame_out_buffer, PIX_FMT_RGB24, _film->format()->dci_width(), _film->format()->dci_height ());

	int const post_filter_width = _video_codec_context->width - _film->left_crop() - _film->right_crop();
	int const post_filter_height = _video_codec_context->height - _film->top_crop() - _film->bottom_crop();

	_conversion_context = sws_getContext (
		post_filter_width, post_filter_height, _video_codec_context->pix_fmt,
		_film->format()->dci_width(), _film->format()->dci_height(), PIX_FMT_RGB24,
		SWS_BICUBIC, 0, 0, 0
		);

	if (_conversion_context == 0) {
		throw runtime_error ("Could not obtain YUV -> RGB conversion context");
	}

	stringstream fs;
	fs << "crop=" << post_filter_width << ":" << post_filter_height << ":" << _film->left_crop() << ":" << _film->top_crop() << " ";
	setup_video_filters (fs.str());
}

void
FilmWriter::setup_audio ()
{
	_audio_codec_context = _format_context->streams[_audio_stream]->codec;
	_audio_codec = avcodec_find_decoder (_audio_codec_context->codec_id);

	if (_audio_codec == 0) {
		throw runtime_error ("Could not find audio decoder");
	}
	
	if (avcodec_open2 (_audio_codec_context, _audio_codec, 0) < 0) {
		throw runtime_error ("Could not open audio decoder");
	}

	/* Create sound output files */
	for (int i = 0; i < _audio_codec_context->channels; ++i) {
		stringstream wav_path;
		wav_path << _wavs << "/" << (i + 1) << ".wav";
		SF_INFO sf_info;
		sf_info.samplerate = _audio_codec_context->sample_rate;
		/* We write mono files */
		sf_info.channels = 1;
		sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
		SNDFILE* f = sf_open (wav_path.str().c_str(), SFM_WRITE, &sf_info);
		if (f == 0) {
			throw runtime_error ("Could not create audio output file");
		}
		_sound_files.push_back (f);
	}

	/* Create buffer for deinterleaving audio */
	_deinterleave_buffer = new uint8_t[_deinterleave_buffer_size];
}


void
FilmWriter::decode ()
{
	while (av_read_frame (_format_context, &_packet) >= 0 && (_nframes == 0 || _frame < _nframes)) {
		if (_packet.stream_index == _video_stream) {
			decode_video ();
		} else if (_packet.stream_index == _audio_stream) {
			decode_audio ();
		}
		av_free_packet (&_packet);
	}
}


void
FilmWriter::decode_video ()
{
	int frame_finished;
	if (avcodec_decode_video2 (_video_codec_context, _frame_in, &frame_finished, &_packet) < 0) {
		throw runtime_error ("Error decoding video");
	}
	
	if (frame_finished) {
		
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
				
				++_frame;
				write_tiff (_tiffs, _frame, _frame_out->data[0], _film->format()->dci_width(), _film->format()->dci_height());
			}
		}
	}
}

void
FilmWriter::decode_audio ()
{
	avcodec_get_frame_defaults (_frame_in);
	
	int frame_finished;
	if (avcodec_decode_audio4 (_audio_codec_context, _frame_in, &frame_finished, &_packet) < 0) {
		throw runtime_error ("Error decoding video");
	}
	
	if (frame_finished) {
		
		int const channels = _audio_codec_context->channels;
		
		int const data_size = av_samples_get_buffer_size (
			0, channels, _frame_in->nb_samples, _audio_codec_context->sample_fmt, 1
			);
		
		/* Size of a sample in bytes */
		int const sample_size = 2;

		/* XXX: we are assuming that sample_size is right, the _deinterleave_buffer_size is a multiple
		   of the sample size and that data_size is a multiple of channels * sample_size.
		*/

		/* XXX: this code is very tricksy and it must be possible to make it simpler ... */
		
		/* Number of bytes left to read this time */
		int remaining = data_size;
		/* Our position in the output buffers, in bytes */
		int position = 0;
		while (remaining > 0) {
			/* How many bytes of the deinterleaved data to do this time */
			int this_time = min (remaining / channels, _deinterleave_buffer_size);
			for (int i = 0; i < channels; ++i) {
				for (int j = 0; j < this_time; j += sample_size) {
					for (int k = 0; k < sample_size; ++k) {
						int const to = j + k;
						int const from = position + (i * sample_size) + (j * channels) + k;
						_deinterleave_buffer[to] = _frame_in->data[0][from];
					}
				}
				
				switch (_audio_codec_context->sample_fmt) {
				case AV_SAMPLE_FMT_S16:
					sf_write_short (_sound_files[i], (const short *) _deinterleave_buffer, this_time / sample_size);
					break;
				default:
					throw runtime_error ("Unknown audio sample format");
				}
			}
			
			position += this_time;
			remaining -= this_time * channels;
		}
	}
}

	
void
FilmWriter::write_tiff (string const & dir, int frame, uint8_t* data, int w, int h) const
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
FilmWriter::setup_video_filters (string const & filters)
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

