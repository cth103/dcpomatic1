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
#include <sigc++/signal.h>
#include "content_type.h"

class Format;
class Job;
class Filter;

class Film
{
public:
	Film (std::string const &, bool must_exist = true);

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

	Format* format () const {
		return _format;
	}

	std::vector<Filter const *> get_filters () const {
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

	std::string dcp_pretty_name () const {
		return _dcp_pretty_name;
	}

	ContentType * dcp_content_type () {
		return _dcp_content_type;
	}

	void set_dcp_frames (int);
	void set_dcp_ab (bool);
	
	void set_name (std::string const &);
	void set_content (std::string const &);
	std::string dir (std::string const &) const;
	std::string file (std::string const &) const;
	void set_top_crop (int);
	void set_bottom_crop (int);
	void set_left_crop (int);
	void set_right_crop (int);
	void set_format (Format *);
	void set_dcp_long_name (std::string const &);
	void set_dcp_pretty_name (std::string const &);
	void set_dcp_content_type (ContentType *);

	int width () const {
		return _width;
	}
	
	int height () const {
		return _height;
	}

	float frames_per_second () const {
		return _frames_per_second;
	}

	std::string j2k_dir () const;
	std::string j2k_path (int, bool) const;

	void update_thumbs_non_gui (Job *);
	void update_thumbs_gui ();
	int num_thumbs () const;
	int thumb_frame (int) const;
	std::string thumb_file (int) const;

	bool dirty () const {
		return _dirty;
	}

	void make_dcp ();

	enum Property {
		Name,
		LeftCrop,
		RightCrop,
		TopCrop,
		BottomCrop,
		Filters,
		Size,
		Content,
		FilmFormat,
		FramesPerSecond,
		DCPLongName,
		DCPPrettyName,
		ContentTypeChange,
		Thumbs,
		DCPFrames,
		DCPAB
	};

	sigc::signal1<void, Property> Changed;
	
private:
	void read_metadata ();
	std::string metadata_file () const;
	void update_dimensions ();
	std::string thumb_file_for_frame (int) const;
	std::string j2k_sub_directory () const;
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
	/** DCP pretty name (e.g. The Blues Brothers) */
	std::string _dcp_pretty_name;
	/** The type of content that this Film represents (feature, trailer etc.) */
	ContentType* _dcp_content_type;
	/** The format to present this Film in (flat, scope, etc.) */
	Format* _format;
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

	/* Vector of frame indices for each of our `thumbnails */
	std::vector<int> _thumbs;
	/* Width, in pixels, of the source */
	int _width;
	/* Height, in pixels, of the source */
	int _height;
	/* Frames per second of the source */
	float _frames_per_second;

	mutable bool _dirty;
};

#endif
