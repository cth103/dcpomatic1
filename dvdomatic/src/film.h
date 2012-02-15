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
		return _directory;
	}
	
	std::string content () const;

	std::string name () const {
		return _name;
	}

	int top_crop () const {
		return _top_crop;
	}

	int bottom_crop () const {
		return _bottom_crop;
	}

	int left_crop () const {
		return _left_crop;
	}

	int right_crop () const {
		return _right_crop;
	}

	Format const * format () const {
		return _format;
	}

	std::vector<Filter const *> filters () const {
		return _filters;
	}

	int dcp_frames () const {
		return _dcp_frames;
	}

	bool dcp_ab () const {
		return _dcp_ab;
	}

	void set_filters (std::vector<Filter const *> const &);

	std::string dcp_long_name () const {
		return _dcp_long_name;
	}

	ContentType const * dcp_content_type () {
		return _dcp_content_type;
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
		return _width;
	}
	
	int height () const {
		return _height;
	}

	int length () const {
		return _length;
	}

	float frames_per_second () const {
		return _frames_per_second;
	}

	int audio_channels () const {
		return _audio_channels;
	}

	int audio_sample_rate () const {
		return _audio_sample_rate;
	}

	AVSampleFormat audio_sample_format () const {
		return _audio_sample_format;
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

	/** Complete path to directory containing the film metadata;
	    must not be relative.
	*/
	std::string _directory;
	/** Name for DVD-o-matic */
	std::string _name;
	/** File containing content (relative to _directory) */
	std::string _content;
	/** DCP long name (e.g. BLUES-BROTHERS_FTR_F_EN-XX ...) */
	std::string _dcp_long_name;
	/** The type of content that this Film represents (feature, trailer etc.) */
	ContentType const * _dcp_content_type;
	/** The format to present this Film in (flat, scope, etc.) */
	Format const * _format;
	/** Number of pixels to crop from the left-hand side of the original picture */
	int _left_crop;
	/** Number of pixels to crop from the right-hand side of the original picture */
	int _right_crop;
	/** Number of pixels to crop from the top of the original picture */
	int _top_crop;
	/** Number of pixels to crop from the bottom of the original picture */
	int _bottom_crop;
	std::vector<Filter const *> _filters;
	/** Number of frames to put in the DCP, or 0 for all */
	int _dcp_frames;
	/** true to create an A/B comparison DCP, where the left half of the image
	    is the video without any filters or post-processing, and the right half
	    has the specified filters and post-processing.
	*/
	bool _dcp_ab;

	/* Data which is cached to speed things up */

	/** Vector of frame indices for each of our `thumbnails */
	std::vector<int> _thumbs;
	/** Width, in pixels, of the source (ignoring cropping) */
	int _width;
	/** Height, in pixels, of the source (ignoring cropping) */
	int _height;
	/** Length in frames */
	int _length;
	/** Frames per second of the source */
	float _frames_per_second;
	/** Number of audio channels */
	int _audio_channels;
	/** Sample rate of the audio, in Hz */
	int _audio_sample_rate;
	/** Format of the audio samples */
	AVSampleFormat _audio_sample_format;

	/** true if our metadata has changed since it was last written to disk */
	mutable bool _dirty;

	Log* _log;
};

#endif
