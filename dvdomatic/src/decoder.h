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

#ifndef DVDOMATIC_DECODER_H
#define DVDOMATIC_DECODER_H

#include <vector>
#include <string>
#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include "util.h"

class Job;
class FilmState;
class Options;
class Image;
class Log;

class Decoder
{
public:
	Decoder (boost::shared_ptr<const FilmState>, boost::shared_ptr<const Options>, Job *, Log *, bool, bool);
	virtual ~Decoder ();

	/* Methods to query our input video */
	virtual int length_in_frames () const = 0;
	/** @return video frames per second, or 0 if unknown */
	virtual float frames_per_second () const = 0;
	virtual Size native_size () const = 0;
	virtual int audio_channels () const = 0;
	virtual int audio_sample_rate () const = 0;
	virtual AVSampleFormat audio_sample_format () const = 0;

	enum PassResult {
		PASS_DONE,    ///< we have decoded all the input
		PASS_NOTHING, ///< nothing new is ready after this pass
		PASS_ERROR,   ///< error; probably temporary
		PASS_VIDEO,   ///< a video frame is available
		PASS_AUDIO    ///< some audio is available
	};

	PassResult pass ();
	void go ();

	int last_video_frame () const {
		return _video_frame;
	}
	
	int decoding_frames () const;
	
	sigc::signal<void, boost::shared_ptr<Image>, int> Video;
	sigc::signal<void, uint8_t *, int, int> Audio;
	
protected:
	virtual PassResult do_pass () = 0;
	
	bool have_video_frame_ready ();
	void emit_video (boost::shared_ptr<Image>);
	void emit_audio (uint8_t *, int, int);
	
	boost::shared_ptr<const FilmState> _fs;
	boost::shared_ptr<const Options> _opt;
	Job* _job;
	Log* _log;

	bool _minimal;
	bool _ignore_length;

private:	
	int _video_frame;
};

#endif
