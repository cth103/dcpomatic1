/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#include "ffmpeg_examiner.h"
#include "ffmpeg_content.h"
#include "job.h"
#include "safe_stringstream.h"

#include "i18n.h"

using std::string;
using std::cout;
using std::max;
using boost::shared_ptr;
using boost::optional;

/** @param job job that the examiner is operating in, or 0 */
FFmpegExaminer::FFmpegExaminer (shared_ptr<const FFmpegContent> c, shared_ptr<Job> job)
	: FFmpeg (c)
	, _video_length (0)
{
	/* Find audio and subtitle streams */

	for (uint32_t i = 0; i < _format_context->nb_streams; ++i) {
		AVStream* s = _format_context->streams[i];
		if (s->codec->codec_type == AVMEDIA_TYPE_AUDIO) {

			/* This is a hack; sometimes it seems that _audio_codec_context->channel_layout isn't set up,
			   so bodge it here.  No idea why we should have to do this.
			*/

			if (s->codec->channel_layout == 0) {
				s->codec->channel_layout = av_get_default_channel_layout (s->codec->channels);
			}
			
			_audio_streams.push_back (
				shared_ptr<FFmpegAudioStream> (
					new FFmpegAudioStream (audio_stream_name (s), s->id, s->codec->sample_rate, s->codec->channels)
					)
				);

		} else if (s->codec->codec_type == AVMEDIA_TYPE_SUBTITLE) {
			_subtitle_streams.push_back (shared_ptr<FFmpegSubtitleStream> (new FFmpegSubtitleStream (subtitle_stream_name (s), s->id)));
		}
	}

	/* See if the header has duration information in it */
	bool const need_video_length = _format_context->duration == AV_NOPTS_VALUE;
	if (!need_video_length) {
		_video_length = double (_format_context->duration) / AV_TIME_BASE;
	} else if (job) {
		job->sub (_("Finding length"));
		job->set_progress_unknown ();
	}

	/* Run through until we find the first audio (for each stream) and video, and also
	   the video length if need_video_length == true.
	*/

	while (true) {
		int r = av_read_frame (_format_context, &_packet);
		if (r < 0) {
			break;
		}

		int frame_finished;

		AVCodecContext* context = _format_context->streams[_packet.stream_index]->codec;

		if (_packet.stream_index == _video_stream) {
			if (avcodec_decode_video2 (context, _frame, &frame_finished, &_packet) >= 0 && frame_finished) {
				if (!_first_video) {
					_first_video = frame_time (_format_context->streams[_video_stream]);
				}
				if (need_video_length) {
					_video_length = frame_time (_format_context->streams[_video_stream]).get_value_or (0);
				}
			}
		} else {
			for (size_t i = 0; i < _audio_streams.size(); ++i) {
				if (_audio_streams[i]->uses_index (_format_context, _packet.stream_index) && !_audio_streams[i]->first_audio) {
					if (avcodec_decode_audio4 (context, _frame, &frame_finished, &_packet) >= 0 && frame_finished) {
						_audio_streams[i]->first_audio = frame_time (_audio_streams[i]->stream (_format_context));
					}
				}
			}
		}

		bool have_all_audio = true;
		size_t i = 0;
		while (i < _audio_streams.size() && have_all_audio) {
			have_all_audio = _audio_streams[i]->first_audio;
			++i;
		}

		av_free_packet (&_packet);
		
		if (_first_video && have_all_audio && !need_video_length) {
			break;
		}
	}
}

optional<double>
FFmpegExaminer::frame_time (AVStream* s) const
{
	optional<double> t;
	
	int64_t const bet = av_frame_get_best_effort_timestamp (_frame);
	if (bet != AV_NOPTS_VALUE) {
		t = bet * av_q2d (s->time_base);
	}

	return t;
}

optional<float>
FFmpegExaminer::video_frame_rate () const
{
	AVStream* s = _format_context->streams[_video_stream];

	if (s->avg_frame_rate.num && s->avg_frame_rate.den) {
		return av_q2d (s->avg_frame_rate);
	}

	return av_q2d (s->r_frame_rate);
}

libdcp::Size
FFmpegExaminer::video_size () const
{
	return libdcp::Size (video_codec_context()->width, video_codec_context()->height);
}

/** @return Length (in video frames) according to our content's header */
VideoContent::Frame
FFmpegExaminer::video_length () const
{
	return max (1, int (rint (_video_length * video_frame_rate().get_value_or (0))));
}

optional<float>
FFmpegExaminer::sample_aspect_ratio () const
{
	AVRational sar = av_guess_sample_aspect_ratio (_format_context, _format_context->streams[_video_stream], 0);
	if (sar.num == 0) {
		/* I assume this means that we don't know */
		return optional<float> ();
	}
	return float (sar.num) / sar.den;
}

string
FFmpegExaminer::audio_stream_name (AVStream* s) const
{
	SafeStringStream n;

	n << stream_name (s);

	if (!n.str().empty()) {
		n << "; ";
	}

	n << s->codec->channels << " channels";

	return n.str ();
}

string
FFmpegExaminer::subtitle_stream_name (AVStream* s) const
{
	SafeStringStream n;

	n << stream_name (s);

	if (n.str().empty()) {
		n << _("unknown");
	}

	return n.str ();
}

string
FFmpegExaminer::stream_name (AVStream* s) const
{
	SafeStringStream n;

	if (s->metadata) {
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
	}

	return n.str ();
}
