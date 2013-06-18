/* -*- c-basic-offset: 8; default-tab-width: 8; -*- */

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

#include <libcxml/cxml.h>
#include "ffmpeg_content.h"
#include "ffmpeg_decoder.h"
#include "compose.hpp"
#include "job.h"
#include "util.h"
#include "filter.h"
#include "log.h"

#include "i18n.h"

using std::string;
using std::stringstream;
using std::vector;
using std::list;
using std::cout;
using boost::shared_ptr;
using boost::lexical_cast;

int const FFmpegContentProperty::SUBTITLE_STREAMS = 100;
int const FFmpegContentProperty::SUBTITLE_STREAM = 101;
int const FFmpegContentProperty::AUDIO_STREAMS = 102;
int const FFmpegContentProperty::AUDIO_STREAM = 103;
int const FFmpegContentProperty::FILTERS = 104;

FFmpegContent::FFmpegContent (shared_ptr<const Film> f, boost::filesystem::path p)
	: Content (f, p)
	, VideoContent (f, p)
	, AudioContent (f, p)
{

}

FFmpegContent::FFmpegContent (shared_ptr<const Film> f, shared_ptr<const cxml::Node> node)
	: Content (f, node)
	, VideoContent (f, node)
	, AudioContent (f, node)
{
	list<shared_ptr<cxml::Node> > c = node->node_children ("SubtitleStream");
	for (list<shared_ptr<cxml::Node> >::const_iterator i = c.begin(); i != c.end(); ++i) {
		_subtitle_streams.push_back (shared_ptr<FFmpegSubtitleStream> (new FFmpegSubtitleStream (*i)));
		if ((*i)->optional_number_child<int> ("Selected")) {
			_subtitle_stream = _subtitle_streams.back ();
		}
	}

	c = node->node_children ("AudioStream");
	for (list<shared_ptr<cxml::Node> >::const_iterator i = c.begin(); i != c.end(); ++i) {
		_audio_streams.push_back (shared_ptr<FFmpegAudioStream> (new FFmpegAudioStream (*i)));
		if ((*i)->optional_number_child<int> ("Selected")) {
			_audio_stream = _audio_streams.back ();
		}
	}

	c = node->node_children ("Filter");
	for (list<shared_ptr<cxml::Node> >::iterator i = c.begin(); i != c.end(); ++i) {
		_filters.push_back (Filter::from_id ((*i)->content ()));
	}
}

FFmpegContent::FFmpegContent (FFmpegContent const & o)
	: Content (o)
	, VideoContent (o)
	, AudioContent (o)
	, _subtitle_streams (o._subtitle_streams)
	, _subtitle_stream (o._subtitle_stream)
	, _audio_streams (o._audio_streams)
	, _audio_stream (o._audio_stream)
{

}

void
FFmpegContent::as_xml (xmlpp::Node* node) const
{
	node->add_child("Type")->add_child_text ("FFmpeg");
	Content::as_xml (node);
	VideoContent::as_xml (node);
	AudioContent::as_xml (node);

	boost::mutex::scoped_lock lm (_mutex);

	for (vector<shared_ptr<FFmpegSubtitleStream> >::const_iterator i = _subtitle_streams.begin(); i != _subtitle_streams.end(); ++i) {
		xmlpp::Node* t = node->add_child("SubtitleStream");
		if (_subtitle_stream && *i == _subtitle_stream) {
			t->add_child("Selected")->add_child_text("1");
		}
		(*i)->as_xml (t);
	}

	for (vector<shared_ptr<FFmpegAudioStream> >::const_iterator i = _audio_streams.begin(); i != _audio_streams.end(); ++i) {
		xmlpp::Node* t = node->add_child("AudioStream");
		if (_audio_stream && *i == _audio_stream) {
			t->add_child("Selected")->add_child_text("1");
		}
		(*i)->as_xml (t);
	}

	for (vector<Filter const *>::const_iterator i = _filters.begin(); i != _filters.end(); ++i) {
		node->add_child("Filter")->add_child_text ((*i)->id ());
	}
}

void
FFmpegContent::examine (shared_ptr<Job> job)
{
	job->set_progress_unknown ();

	Content::examine (job);

	shared_ptr<const Film> film = _film.lock ();
	assert (film);

	shared_ptr<FFmpegDecoder> decoder (new FFmpegDecoder (film, shared_from_this (), true, false));

	ContentVideoFrame video_length = 0;
	video_length = decoder->video_length ();
	film->log()->log (String::compose ("Video length obtained from header as %1 frames", decoder->video_length ()));

        {
                boost::mutex::scoped_lock lm (_mutex);

                _video_length = video_length;

                _subtitle_streams = decoder->subtitle_streams ();
                if (!_subtitle_streams.empty ()) {
                        _subtitle_stream = _subtitle_streams.front ();
                }
                
                _audio_streams = decoder->audio_streams ();
                if (!_audio_streams.empty ()) {
                        _audio_stream = _audio_streams.front ();
                }
        }

        take_from_video_decoder (decoder);

        signal_changed (ContentProperty::LENGTH);
        signal_changed (FFmpegContentProperty::SUBTITLE_STREAMS);
        signal_changed (FFmpegContentProperty::SUBTITLE_STREAM);
        signal_changed (FFmpegContentProperty::AUDIO_STREAMS);
        signal_changed (FFmpegContentProperty::AUDIO_STREAM);
        signal_changed (AudioContentProperty::AUDIO_CHANNELS);
}

string
FFmpegContent::summary () const
{
	return String::compose (_("Movie: %1"), file().filename().string());
}

string
FFmpegContent::information () const
{
	if (video_length() == 0 || video_frame_rate() == 0) {
		return "";
	}
	
	stringstream s;
	
	s << String::compose (_("%1 frames; %2 frames per second"), video_length(), video_frame_rate()) << "\n";
	s << VideoContent::information ();

	return s.str ();
}

void
FFmpegContent::set_subtitle_stream (shared_ptr<FFmpegSubtitleStream> s)
{
        {
                boost::mutex::scoped_lock lm (_mutex);
                _subtitle_stream = s;
        }

        signal_changed (FFmpegContentProperty::SUBTITLE_STREAM);
}

void
FFmpegContent::set_audio_stream (shared_ptr<FFmpegAudioStream> s)
{
        {
                boost::mutex::scoped_lock lm (_mutex);
                _audio_stream = s;
        }

        signal_changed (FFmpegContentProperty::AUDIO_STREAM);
}

ContentAudioFrame
FFmpegContent::audio_length () const
{
	int const cafr = content_audio_frame_rate ();
	int const vfr  = video_frame_rate ();
	ContentVideoFrame const vl = video_length ();

	boost::mutex::scoped_lock lm (_mutex);
        if (!_audio_stream) {
                return 0;
        }
        
        return video_frames_to_audio_frames (vl, cafr, vfr);
}

int
FFmpegContent::audio_channels () const
{
	boost::mutex::scoped_lock lm (_mutex);
	
        if (!_audio_stream) {
                return 0;
        }

        return _audio_stream->channels;
}

int
FFmpegContent::content_audio_frame_rate () const
{
	boost::mutex::scoped_lock lm (_mutex);

        if (!_audio_stream) {
                return 0;
        }

        return _audio_stream->frame_rate;
}

int
FFmpegContent::output_audio_frame_rate () const
{
	shared_ptr<const Film> film = _film.lock ();
	assert (film);
	
	/* Resample to a DCI-approved sample rate */
	double t = dcp_audio_frame_rate (content_audio_frame_rate ());

	FrameRateConversion frc (video_frame_rate(), film->dcp_video_frame_rate());

	/* Compensate if the DCP is being run at a different frame rate
	   to the source; that is, if the video is run such that it will
	   look different in the DCP compared to the source (slower or faster).
	   skip/repeat doesn't come into effect here.
	*/

	if (frc.change_speed) {
		t *= video_frame_rate() * frc.factor() / film->dcp_video_frame_rate();
	}

	return rint (t);
}

bool
operator== (FFmpegSubtitleStream const & a, FFmpegSubtitleStream const & b)
{
        return a.id == b.id;
}

bool
operator== (FFmpegAudioStream const & a, FFmpegAudioStream const & b)
{
        return a.id == b.id;
}

FFmpegAudioStream::FFmpegAudioStream (shared_ptr<const cxml::Node> node)
{
	name = node->string_child ("Name");
	id = node->number_child<int> ("Id");
	frame_rate = node->number_child<int> ("FrameRate");
	channels = node->number_child<int64_t> ("Channels");
	mapping = AudioMapping (node->node_child ("Mapping"));
}

void
FFmpegAudioStream::as_xml (xmlpp::Node* root) const
{
	root->add_child("Name")->add_child_text (name);
	root->add_child("Id")->add_child_text (lexical_cast<string> (id));
	root->add_child("FrameRate")->add_child_text (lexical_cast<string> (frame_rate));
	root->add_child("Channels")->add_child_text (lexical_cast<string> (channels));
	mapping.as_xml (root->add_child("Mapping"));
}

/** Construct a SubtitleStream from a value returned from to_string().
 *  @param t String returned from to_string().
 *  @param v State file version.
 */
FFmpegSubtitleStream::FFmpegSubtitleStream (shared_ptr<const cxml::Node> node)
{
	name = node->string_child ("Name");
	id = node->number_child<int> ("Id");
}

void
FFmpegSubtitleStream::as_xml (xmlpp::Node* root) const
{
	root->add_child("Name")->add_child_text (name);
	root->add_child("Id")->add_child_text (lexical_cast<string> (id));
}

shared_ptr<Content>
FFmpegContent::clone () const
{
	return shared_ptr<Content> (new FFmpegContent (*this));
}

Time
FFmpegContent::length () const
{
	shared_ptr<const Film> film = _film.lock ();
	assert (film);
	
	FrameRateConversion frc (video_frame_rate (), film->dcp_video_frame_rate ());
	return video_length() * frc.factor() * TIME_HZ / film->dcp_video_frame_rate ();
}

AudioMapping
FFmpegContent::audio_mapping () const
{
	boost::mutex::scoped_lock lm (_mutex);

	if (!_audio_stream) {
		return AudioMapping ();
	}

	return _audio_stream->mapping;
}

void
FFmpegContent::set_filters (vector<Filter const *> const & filters)
{
	{
		boost::mutex::scoped_lock lm (_mutex);
		_filters = filters;
	}

	signal_changed (FFmpegContentProperty::FILTERS);
}

void
FFmpegContent::set_audio_mapping (AudioMapping m)
{
	audio_stream()->mapping = m;
	signal_changed (AudioContentProperty::AUDIO_MAPPING);
}