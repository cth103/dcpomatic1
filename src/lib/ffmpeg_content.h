/*
    Copyright (C) 2013-2015 Carl Hetherington <cth@carlh.net>

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

#ifndef DCPOMATIC_FFMPEG_CONTENT_H
#define DCPOMATIC_FFMPEG_CONTENT_H

#include <boost/enable_shared_from_this.hpp>
#include "video_content.h"
#include "audio_content.h"
#include "subtitle_content.h"
#include "audio_mapping.h"
#include "audio_stream.h"

struct AVFormatContext;
struct AVStream;

class Filter;
class ffmpeg_pts_offset_test;

class FFmpegStream
{
public:
	FFmpegStream (std::string n, int i)
		: name (n)
		, _id (i)
	{}
				
	FFmpegStream (boost::shared_ptr<const cxml::Node>);

	void as_xml (xmlpp::Node *) const;

	/** @param c An AVFormatContext.
	 *  @param index A stream index within the AVFormatContext.
	 *  @return true if this FFmpegStream uses the given stream index.
	 */
	bool uses_index (AVFormatContext const * c, int index) const;
	AVStream* stream (AVFormatContext const * c) const;

	std::string technical_summary () const {
		return "id " + boost::lexical_cast<std::string> (_id);
	}

	std::string identifier () const {
		return boost::lexical_cast<std::string> (_id);
	}

	std::string name;

	friend bool operator== (FFmpegStream const & a, FFmpegStream const & b);
	friend bool operator!= (FFmpegStream const & a, FFmpegStream const & b);
	
private:
	int _id;
};

class FFmpegAudioStream : public FFmpegStream, public AudioStream
{
public:
	FFmpegAudioStream (std::string n, int i, int frame_rate, int channels)
		: FFmpegStream (n, i)
		, AudioStream (frame_rate, channels)
	{}

	FFmpegAudioStream (boost::shared_ptr<const cxml::Node>, int);

	void as_xml (xmlpp::Node *) const;

	/* BXXX: this should probably be locked */

	boost::optional<double> first_audio;

private:
	friend class ffmpeg_pts_offset_test;

	/* Constructor for tests */
	FFmpegAudioStream ()
		: FFmpegStream ("", 0)
		, AudioStream (0, 0)
	{}
};

class FFmpegSubtitleStream : public FFmpegStream
{
public:
	FFmpegSubtitleStream (std::string n, int i)
		: FFmpegStream (n, i)
	{}
	
	FFmpegSubtitleStream (boost::shared_ptr<const cxml::Node>);

	void as_xml (xmlpp::Node *) const;
};

class FFmpegContentProperty : public VideoContentProperty
{
public:
	static int const SUBTITLE_STREAMS;
	static int const SUBTITLE_STREAM;
	static int const AUDIO_STREAMS;
	static int const FILTERS;
};

class FFmpegContent : public VideoContent, public AudioContent, public SubtitleContent
{
public:
	FFmpegContent (boost::shared_ptr<const Film>, boost::filesystem::path);
	FFmpegContent (boost::shared_ptr<const Film>, boost::shared_ptr<const cxml::Node>, int version, std::list<std::string> &);
	FFmpegContent (boost::shared_ptr<const Film>, std::vector<boost::shared_ptr<Content> >);

	boost::shared_ptr<FFmpegContent> shared_from_this () {
		return boost::dynamic_pointer_cast<FFmpegContent> (Content::shared_from_this ());
	}
	
	void examine (boost::shared_ptr<Job>);
	std::string summary () const;
	std::string technical_summary () const;
	void as_xml (xmlpp::Node *) const;
	Time full_length () const;

	std::string identifier () const;
	
	/* AudioContent */
	AudioContent::Frame audio_length () const;
	std::vector<AudioStreamPtr> audio_streams () const;

	void set_filters (std::vector<Filter const *> const &);
	
	std::vector<boost::shared_ptr<FFmpegSubtitleStream> > subtitle_streams () const {
		boost::mutex::scoped_lock lm (_mutex);
		return _subtitle_streams;
	}

	boost::shared_ptr<FFmpegSubtitleStream> subtitle_stream () const {
		boost::mutex::scoped_lock lm (_mutex);
		return _subtitle_stream;
	}

	std::vector<boost::shared_ptr<FFmpegAudioStream> > ffmpeg_audio_streams () const {
		boost::mutex::scoped_lock lm (_mutex);
		return _audio_streams;
	}

	std::vector<Filter const *> filters () const {
		boost::mutex::scoped_lock lm (_mutex);
		return _filters;
	}

	void set_subtitle_stream (boost::shared_ptr<FFmpegSubtitleStream>);

	boost::optional<double> first_video () const {
		boost::mutex::scoped_lock lm (_mutex);
		return _first_video;
	}

private:
	friend class ffmpeg_pts_offset_test;
	friend class audio_sampling_rate_test;
	
	std::vector<boost::shared_ptr<FFmpegSubtitleStream> > _subtitle_streams;
	boost::shared_ptr<FFmpegSubtitleStream> _subtitle_stream;
	std::vector<boost::shared_ptr<FFmpegAudioStream> > _audio_streams;
	boost::optional<double> _first_video;
	/** Video filters that should be used when generating DCPs */
	std::vector<Filter const *> _filters;
};

#endif
