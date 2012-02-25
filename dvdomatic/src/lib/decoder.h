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
#include <sigc++/sigc++.h>
#include "util.h"

class Job;
class FilmState;
class Options;
class Image;
class Log;

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
	Decoder (boost::shared_ptr<const FilmState>, boost::shared_ptr<const Options>, Job *, Log *, bool, bool);
	virtual ~Decoder () {}

	/* Methods to query our input video */

	/** @return length in video frames */
	virtual int length_in_frames () const = 0;
	/** @return video frames per second, or 0 if unknown */
	virtual float frames_per_second () const = 0;
	/** @return native size in pixels */
	virtual Size native_size () const = 0;
	/** @return number of audio channels */
	virtual int audio_channels () const = 0;
	/** @return audio sampling rate in Hz */
	virtual int audio_sample_rate () const = 0;
	/** @return format of audio samples */
	virtual AVSampleFormat audio_sample_format () const = 0;

	/** Result of a call to pass() */
	enum PassResult {
		PASS_DONE,    ///< we have decoded all the input
		PASS_NOTHING, ///< nothing new is ready after this pass
		PASS_ERROR,   ///< error; probably temporary
		PASS_VIDEO,   ///< a video frame is available
		PASS_AUDIO    ///< some audio is available
	};

	PassResult pass ();
	void go ();

	/** @return the index of the last video frame to be processed */
	int last_video_frame () const {
		return _video_frame;
	}
	
	int decoding_frames () const;

	/** Emitted when a video frame is ready.
	 *  First parameter is the frame.
	 *  Second parameter is its index within the content.
	 */
	sigc::signal<void, boost::shared_ptr<Image>, int> Video;

	/** Emitted when some audio data is ready.
	 *  First parameter is the interleaved sample data, format is given in the FilmState.
	 *  Second parameter is the number of channels in the data.
	 *  Third parameter is the size of the data.
	 */
	sigc::signal<void, uint8_t *, int, int> Audio;
	
protected:
	/** perform a single pass at our content */
	virtual PassResult do_pass () = 0;
	
	bool have_video_frame_ready ();
	void emit_video (boost::shared_ptr<Image>);
	void emit_audio (uint8_t *, int, int);

	/** our FilmState */
	boost::shared_ptr<const FilmState> _fs;
	/** our options */
	boost::shared_ptr<const Options> _opt;
	/** associated Job, or 0 */
	Job* _job;
	/** log that we can write to */
	Log* _log;

	/** true to do the bare minimum of work; just run through the content.  Useful for acquiring
	 *  accurate frame counts as quickly as possible.  This generates no video or audio output.
	 */
	bool _minimal;

	/** ignore_length Ignore the content's claimed length when computing progress */
	bool _ignore_length;

private:
	/** last video frame to be processed */
	int _video_frame;
};

#endif
