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

#ifndef DVDOMATIC_FILM_H
#define DVDOMATIC_FILM_H

#include <string>
#include <vector>
#include <inttypes.h>
#include <boost/thread/mutex.hpp>
#include <sigc++/signal.h>
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "content_type.h"
#include "film_state.h"

class Format;
class Job;
class Filter;
class Log;
class ExamineContentJob;

/** A representation of a piece of video (with sound), including naming,
 *  the source content file, and how it should be presented in a DCP.
 */
class Film
{
public:
	Film (std::string, bool must_exist = true);
	Film (Film const &);
	~Film ();

	void write_metadata () const;

	std::string directory () const {
		return _state.directory;
	}
	
	std::string content () const;

	std::string name () const {
		return _state.name;
	}

	int top_crop () const {
		return _state.top_crop;
	}

	int bottom_crop () const {
		return _state.bottom_crop;
	}

	int left_crop () const {
		return _state.left_crop;
	}

	int right_crop () const {
		return _state.right_crop;
	}

	Format const * format () const {
		return _state.format;
	}

	std::vector<Filter const *> filters () const {
		return _state.filters;
	}

	Scaler const * scaler () const {
		return _state.scaler;
	}

	int dcp_frames () const {
		return _state.dcp_frames;
	}

	bool dcp_ab () const {
		return _state.dcp_ab;
	}

	void set_filters (std::vector<Filter const *> const &);

	void set_scaler (Scaler const *);

	std::string dcp_long_name () const {
		return _state.dcp_long_name;
	}

	bool guess_dcp_long_name () const {
		return _state.guess_dcp_long_name;
	}

	ContentType const * dcp_content_type () {
		return _state.dcp_content_type;
	}

	void set_dcp_frames (int);
	void set_dcp_ab (bool);
	
	void set_name (std::string);
	void set_content (std::string);
	void set_top_crop (int);
	void set_bottom_crop (int);
	void set_left_crop (int);
	void set_right_crop (int);
	void set_frames_per_second (float);
	void set_format (Format *);
	void set_dcp_long_name (std::string);
	void set_guess_dcp_long_name (bool);
	void set_dcp_content_type (ContentType const *);

	Size size () const {
		return _state.size;
	}
	
	int length () const {
		return _state.length;
	}

	float frames_per_second () const {
		return _state.frames_per_second;
	}

	int audio_channels () const {
		return _state.audio_channels;
	}

	int audio_sample_rate () const {
		return _state.audio_sample_rate;
	}

	AVSampleFormat audio_sample_format () const {
		return _state.audio_sample_format;
	}
	
	std::string j2k_dir () const;

	void update_thumbs_pre_gui ();
	void update_thumbs_post_gui ();
	int num_thumbs () const;
	int thumb_frame (int) const;
	std::string thumb_file (int) const;

	void copy_from_dvd_post_gui ();
	void examine_content ();
	void examine_content_post_gui ();

	bool dirty () const {
		return _dirty;
	}

	void make_dcp (int freq = 0);

	enum Property {
		Name,
		Content,
		DCPLongName,
		GuessDCPLongName,
		DCPContentType,
		FilmFormat,
		LeftCrop,
		RightCrop,
		TopCrop,
		BottomCrop,
		Filters,
		FilmScaler,
		DCPFrames,
		DCPAB,
		Thumbs,
		FilmSize,
		Length,
		FramesPerSecond,
		AudioChannels,
		AudioSampleRate
	};

	boost::shared_ptr<FilmState> state_copy () const;

	/** @return Logger.
	 *  It is safe to call this from any thread.
	 */
	Log* log () const {
		return _log;
	}

	/** Emitted when some metadata property has changed */
	sigc::signal1<void, Property> Changed;
	
private:
	void read_metadata ();
	std::string metadata_file () const;
	void update_dimensions ();
	void signal_changed (Property);
	void maybe_guess_dcp_long_name ();

	FilmState _state;

	/** true if our metadata has changed since it was last written to disk */
	mutable bool _dirty;

	Log* _log;

	boost::shared_ptr<ExamineContentJob> _examine_content_job;
};

#endif
