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
#include <boost/lexical_cast.hpp>
extern "C" {
#include <tiffio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libpostproc/postprocess.h>
}
#include <sndfile.h>
#include "film.h"
#include "format.h"
#include "transcoder.h"
#include "job.h"
#include "filter.h"
#include "options.h"
#include "exceptions.h"
#include "image.h"
#include "util.h"
#include "log.h"
#include "ffmpeg_decoder.h"
#include "subtitle.h"

using std::cout;
using std::string;
using std::vector;
using std::stringstream;
using boost::shared_ptr;

FFmpegDecoder::FFmpegDecoder (shared_ptr<Film> f, shared_ptr<const Options> o, Job* j, bool minimal)
	: Decoder (f, o, j, minimal)
	, _format_context (0)
	, _video_stream (-1)
	, _audio_stream (-1)
	, _subtitle_stream (-1)
	, _frame (0)
	, _video_codec_context (0)
	, _video_codec (0)
	, _audio_codec_context (0)
	, _audio_codec (0)
	, _subtitle_codec_context (0)
	, _subtitle_codec (0)
{
	setup_general ();
	setup_video ();
	setup_audio ();
	setup_subtitle ();
}

FFmpegDecoder::~FFmpegDecoder ()
{
	if (_audio_codec_context) {
		avcodec_close (_audio_codec_context);
	}
	
	if (_video_codec_context) {
		avcodec_close (_video_codec_context);
	}

	if (_subtitle_codec_context) {
		avcodec_close (_subtitle_codec_context);
	}

	av_free (_frame);
	
	avformat_close_input (&_format_context);
}	

void
FFmpegDecoder::setup_general ()
{
	int r;
	
	av_register_all ();

	if ((r = avformat_open_input (&_format_context, _film->content_path().c_str(), 0, 0)) != 0) {
		throw OpenFileError (_film->content_path ());
	}

	if (avformat_find_stream_info (_format_context, 0) < 0) {
		throw DecodeError ("could not find stream information");
	}

	/* Find video, audio and subtitle streams and choose the first of each */

	for (uint32_t i = 0; i < _format_context->nb_streams; ++i) {
		AVStream* s = _format_context->streams[i];
		if (s->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			_video_stream = i;
		} else if (s->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			if (_audio_stream == -1) {
				_audio_stream = i;
			}
			_audio_streams.push_back (AudioStream (stream_name (s), i, s->codec->channels));
		} else if (s->codec->codec_type == AVMEDIA_TYPE_SUBTITLE) {
			if (_subtitle_stream == -1) {
				_subtitle_stream = i;
			}
			_subtitle_streams.push_back (SubtitleStream (stream_name (s), i));
		}
	}

	/* Now override audio and subtitle streams with those from the Film, if it has any */

	if (_film->audio_stream_index() != -1) {
		_audio_stream = _film->audio_stream().id();
	}

	if (_film->subtitle_stream_index() != -1) {
		_subtitle_stream = _film->subtitle_stream().id ();
	}

	if (_video_stream < 0) {
		throw DecodeError ("could not find video stream");
	}

	_frame = avcodec_alloc_frame ();
	if (_frame == 0) {
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

	/* I think this prevents problems with green hash on decodes and
	   "changing frame properties on the fly is not supported by all filters"
	   messages with some content.  Although I'm not sure; needs checking.
	*/
	AVDictionary* opts = 0;
	av_dict_set (&opts, "threads", "1", 0);
	
	if (avcodec_open2 (_video_codec_context, _video_codec, &opts) < 0) {
		throw DecodeError ("could not open video decoder");
	}
}

void
FFmpegDecoder::setup_audio ()
{
	if (_audio_stream < 0) {
		return;
	}
	
	_audio_codec_context = _format_context->streams[_audio_stream]->codec;
	_audio_codec = avcodec_find_decoder (_audio_codec_context->codec_id);

	if (_audio_codec == 0) {
		throw DecodeError ("could not find audio decoder");
	}

	if (avcodec_open2 (_audio_codec_context, _audio_codec, 0) < 0) {
		throw DecodeError ("could not open audio decoder");
	}

	/* This is a hack; sometimes it seems that _audio_codec_context->channel_layout isn't set up,
	   so bodge it here.  No idea why we should have to do this.
	*/

	if (_audio_codec_context->channel_layout == 0) {
		_audio_codec_context->channel_layout = av_get_default_channel_layout (audio_channels ());
	}
}

void
FFmpegDecoder::setup_subtitle ()
{
	if (_subtitle_stream < 0) {
		return;
	}

	_subtitle_codec_context = _format_context->streams[_subtitle_stream]->codec;
	_subtitle_codec = avcodec_find_decoder (_subtitle_codec_context->codec_id);

	if (_subtitle_codec == 0) {
		throw DecodeError ("could not find subtitle decoder");
	}
	
	if (avcodec_open2 (_subtitle_codec_context, _subtitle_codec, 0) < 0) {
		throw DecodeError ("could not open subtitle decoder");
	}
}


bool
FFmpegDecoder::pass ()
{
	int r = av_read_frame (_format_context, &_packet);
	
	if (r < 0) {
		if (r != AVERROR_EOF) {
			/* Maybe we should fail here, but for now we'll just finish off instead */
			char buf[256];
			av_strerror (r, buf, sizeof(buf));
			_film->log()->log (String::compose ("error on av_read_frame (%1) (%2)", buf, r));
		}
		
		/* Get any remaining frames */
		
		_packet.data = 0;
		_packet.size = 0;

		/* XXX: should we reset _packet.data and size after each *_decode_* call? */

		int frame_finished;

		while (avcodec_decode_video2 (_video_codec_context, _frame, &frame_finished, &_packet) >= 0 && frame_finished) {
			process_video (_frame);
		}

		if (_audio_stream >= 0 && _opt->decode_audio) {
			while (avcodec_decode_audio4 (_audio_codec_context, _frame, &frame_finished, &_packet) >= 0 && frame_finished) {
				int const data_size = av_samples_get_buffer_size (
					0, _audio_codec_context->channels, _frame->nb_samples, audio_sample_format (), 1
					);

				assert (_audio_codec_context->channels == _film->audio_channels());
				process_audio (_frame->data[0], data_size);
			}
		}

		return true;
	}

	avcodec_get_frame_defaults (_frame);
	
	if (_packet.stream_index == _video_stream) {

		int frame_finished;
		int const r = avcodec_decode_video2 (_video_codec_context, _frame, &frame_finished, &_packet);
		if (r >= 0 && frame_finished) {

			if (r != _packet.size) {
				_film->log()->log (String::compose ("Used only %1 bytes of %2 in packet", r, _packet.size));
			}

			/* Where we are in the output, in seconds */
			double const out_pts_seconds = video_frames_in() / frames_per_second();

			/* Where we are in the source, in seconds */
			double const source_pts_seconds = av_q2d (_format_context->streams[_packet.stream_index]->time_base)
				* av_frame_get_best_effort_timestamp(_frame);

			if (!_first_video) {
				_first_video = source_pts_seconds;
			}

			/* Difference between where we are and where we should be */
			double const delta = source_pts_seconds - _first_video.get() - out_pts_seconds;
			double const one_frame = 1 / frames_per_second();

			/* Insert frames if required to get out_pts_seconds up to pts_seconds */
			if (delta > one_frame) {
				int const extra = rint (delta / one_frame);
				for (int i = 0; i < extra; ++i) {
					repeat_last_video ();
					_film->log()->log (
						String::compose (
							"Extra frame inserted at %1s; source frame %2, source PTS %3",
							out_pts_seconds, video_frames_in(), source_pts_seconds
							)
						);
				}
			}

			if (delta > -one_frame) {
				/* Process this frame */
				process_video (_frame);
			} else {
				/* Otherwise we are omitting a frame to keep things right */
				_film->log()->log (String::compose ("Frame removed at %1s", out_pts_seconds));
			}
		}

	} else if (_audio_stream >= 0 && _packet.stream_index == _audio_stream && _opt->decode_audio) {

		int frame_finished;
		if (avcodec_decode_audio4 (_audio_codec_context, _frame, &frame_finished, &_packet) >= 0 && frame_finished) {

			/* Where we are in the source, in seconds */
			double const source_pts_seconds = av_q2d (_format_context->streams[_packet.stream_index]->time_base)
				* av_frame_get_best_effort_timestamp(_frame);

			/* We only decode audio if we've had our first video packet through, and if it
			   was before this packet.  Until then audio is thrown away.
			*/
				
			if (_first_video && _first_video.get() <= source_pts_seconds) {

				if (!_first_audio) {
					_first_audio = source_pts_seconds;
					
					/* This is our first audio frame, and if we've arrived here we must have had our
					   first video frame.  Push some silence to make up any gap between our first
					   video frame and our first audio.
					*/
			
					/* frames of silence that we must push */
					int const s = rint ((_first_audio.get() - _first_video.get()) * audio_sample_rate ());
					
					_film->log()->log (
						String::compose (
							"First video at %1, first audio at %2, pushing %3 frames of silence for %4 channels (%5 bytes per sample)",
							_first_video.get(), _first_audio.get(), s, audio_channels(), bytes_per_audio_sample()
							)
						);
					
					if (s) {
						/* hence bytes */
						int const b = s * audio_channels() * bytes_per_audio_sample();
						
						/* XXX: this assumes that it won't be too much, and there are shaky assumptions
						   that all sound representations are silent with memset()ed zero data.
						*/
						uint8_t silence[b];
						memset (silence, 0, b);
						process_audio (silence, b);
					}
				}

				int const data_size = av_samples_get_buffer_size (
					0, _audio_codec_context->channels, _frame->nb_samples, audio_sample_format (), 1
					);
				
				assert (_audio_codec_context->channels == _film->audio_channels());
				process_audio (_frame->data[0], data_size);
			}
		}
			
	} else if (_subtitle_stream >= 0 && _packet.stream_index == _subtitle_stream && _opt->decode_subtitles && _first_video) {

		int got_subtitle;
		AVSubtitle sub;
		if (avcodec_decode_subtitle2 (_subtitle_codec_context, &sub, &got_subtitle, &_packet) && got_subtitle) {
			/* Sometimes we get an empty AVSubtitle, which is used by some codecs to
			   indicate that the previous subtitle should stop.
			*/
			if (sub.num_rects > 0) {
				process_subtitle (shared_ptr<TimedSubtitle> (new TimedSubtitle (sub, _first_video.get())));
			} else {
				process_subtitle (shared_ptr<TimedSubtitle> ());
			}
			avsubtitle_free (&sub);
		}
	}
	
	av_free_packet (&_packet);
	return false;
}

float
FFmpegDecoder::frames_per_second () const
{
	AVStream* s = _format_context->streams[_video_stream];

	if (s->avg_frame_rate.num && s->avg_frame_rate.den) {
		return av_q2d (s->avg_frame_rate);
	}

	return av_q2d (s->r_frame_rate);
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
	if (_audio_codec_context == 0) {
		return (AVSampleFormat) 0;
	}
	
	return _audio_codec_context->sample_fmt;
}

int64_t
FFmpegDecoder::audio_channel_layout () const
{
	if (_audio_codec_context == 0) {
		return 0;
	}
	
	return _audio_codec_context->channel_layout;
}

Size
FFmpegDecoder::native_size () const
{
	return Size (_video_codec_context->width, _video_codec_context->height);
}

PixelFormat
FFmpegDecoder::pixel_format () const
{
	return _video_codec_context->pix_fmt;
}

int
FFmpegDecoder::time_base_numerator () const
{
	return _video_codec_context->time_base.num;
}

int
FFmpegDecoder::time_base_denominator () const
{
	return _video_codec_context->time_base.den;
}

int
FFmpegDecoder::sample_aspect_ratio_numerator () const
{
	return _video_codec_context->sample_aspect_ratio.num;
}

int
FFmpegDecoder::sample_aspect_ratio_denominator () const
{
	return _video_codec_context->sample_aspect_ratio.den;
}

bool
FFmpegDecoder::has_subtitles () const
{
	return (_subtitle_stream != -1);
}

vector<AudioStream>
FFmpegDecoder::audio_streams () const
{
	return _audio_streams;
}

vector<SubtitleStream>
FFmpegDecoder::subtitle_streams () const
{
	return _subtitle_streams;
}

string
FFmpegDecoder::stream_name (AVStream* s) const
{
	stringstream n;
	
	AVDictionaryEntry const * lang = av_dict_get (s->metadata, "language", 0, 0);
	if (lang) {
		n << lang->value;
	}
	
	AVDictionaryEntry const * title = av_dict_get (s->metadata, "title", 0, 0);
	if (title) {
		if (!n.str().empty()) {
			n << " ";
		}
		n << title->value;
	}

	if (n.str().empty()) {
		n << "unknown";
	}

	return n.str ();
}

