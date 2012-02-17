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

class Format;
class Job;
class Filter;
class Log;

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

	int dcp_frames () const {
		return _state.dcp_frames;
	}

	bool dcp_ab () const {
		return _state.dcp_ab;
	}

	void set_filters (std::vector<Filter const *> const &);

	std::string dcp_long_name () const {
		return _state.dcp_long_name;
	}

	ContentType const * dcp_content_type () {
		return _state.dcp_content_type;
	}

	void set_dcp_frames (int);
	void set_dcp_ab (bool);
	
	void set_name (std::string);
	void set_content (std::string);
	std::string dir (std::string) const;
	std::string file (std::string) const;
	void set_top_crop (int);
	void set_bottom_crop (int);
	void set_left_crop (int);
	void set_right_crop (int);
	void set_format (Format *);
	void set_dcp_long_name (std::string);
	void set_dcp_content_type (ContentType const *);

	int width () const {
		return _state.width;
	}
	
	int height () const {
		return _state.height;
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

	bool dirty () const {
		return _dirty;
	}

	void make_dcp ();

	enum Property {
		Name,
		Content,
		DCPLongName,
		DCPContentType,
		FilmFormat,
		LeftCrop,
		RightCrop,
		TopCrop,
		BottomCrop,
		Filters,
		DCPFrames,
		DCPAB,
		Thumbs,
		Size,
		Length,
		FramesPerSecond,
		AudioChannels,
		AudioSampleRate
	};

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
	std::string thumb_file_for_frame (int) const;
	void signal_changed (Property);

	FilmState _state;

	/** true if our metadata has changed since it was last written to disk */
	mutable bool _dirty;

	Log* _log;
};

#endif
