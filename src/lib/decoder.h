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

/** @file  src/decoder.h
 *  @brief Parent class for decoders of content.
 */

#ifndef DVDOMATIC_DECODER_H
#define DVDOMATIC_DECODER_H

#include <vector>
#include <string>
#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>
#include "util.h"
#include "stream.h"

class Job;
class Options;
class Image;
class Log;
class DelayLine;
class TimedSubtitle;
class Subtitle;
class Film;
class FilterGraph;

/** @class Decoder.
 *  @brief Parent class for decoders of content.
 *
 *  These classes can be instructed run through their content
 *  (by calling ::go), and they emit signals when video or audio data is ready for something else
 *  to process.
 */
class Decoder
{
public:
	Decoder (boost::shared_ptr<Film>, boost::shared_ptr<const Options>, Job *);
	virtual ~Decoder ();

	/* Methods to query our input video */

	/** @return video frames per second, or 0 if unknown */
	virtual float frames_per_second () const = 0;
	/** @return native size in pixels */
	virtual Size native_size () const = 0;

	virtual int time_base_numerator () const = 0;
	virtual int time_base_denominator () const = 0;
	virtual int sample_aspect_ratio_numerator () const = 0;
	virtual int sample_aspect_ratio_denominator () const = 0;
	
	void process_begin ();
	virtual bool pass () = 0;
	void process_end ();
	void go ();

	SourceFrame video_frame () const {
		return _video_frame;
	}

	virtual void set_audio_stream (boost::optional<AudioStream>);
	virtual void set_subtitle_stream (boost::optional<SubtitleStream>);

	boost::optional<AudioStream> audio_stream () const {
		return _audio_stream;
	}

	boost::optional<SubtitleStream> subtitle_stream () const {
		return _subtitle_stream;
	}

	std::vector<AudioStream> audio_streams () const {
		return _audio_streams;
	}
	
	std::vector<SubtitleStream> subtitle_streams () const {
		return _subtitle_streams;
	}

	/** Emitted when a video frame is ready.
	 *  First parameter is the frame within the source.
	 *  Second parameter is its index within the content.
	 *  Third parameter is either 0 or a subtitle that should be on this frame.
	 */
	boost::signals2::signal<void (boost::shared_ptr<Image>, SourceFrame, boost::shared_ptr<Subtitle>)> Video;

	/** Emitted when some audio data is ready */
	boost::signals2::signal<void (boost::shared_ptr<AudioBuffers>, int64_t)> Audio;

protected:
	
	virtual PixelFormat pixel_format () const = 0;
	
	void process_video (AVFrame *);
	void process_audio (boost::shared_ptr<AudioBuffers>);
	void process_subtitle (boost::shared_ptr<TimedSubtitle>);
	void repeat_last_video ();

	/** our Film */
	boost::shared_ptr<Film> _film;
	/** our options */
	boost::shared_ptr<const Options> _opt;
	/** associated Job, or 0 */
	Job* _job;

	boost::optional<AudioStream> _audio_stream;
	boost::optional<SubtitleStream> _subtitle_stream;

	std::vector<AudioStream> _audio_streams;
	std::vector<SubtitleStream> _subtitle_streams;
	
private:
	void emit_video (boost::shared_ptr<Image>, boost::shared_ptr<Subtitle>);
	void emit_audio (boost::shared_ptr<AudioBuffers>);

	SourceFrame _video_frame;
	int64_t _audio_frame;

	std::list<boost::shared_ptr<FilterGraph> > _filter_graphs;

	DelayLine* _delay_line;
	int _delay_in_frames;

	boost::shared_ptr<TimedSubtitle> _timed_subtitle;

	boost::shared_ptr<Image> _last_image;
	boost::shared_ptr<Subtitle> _last_subtitle;
};

#endif
