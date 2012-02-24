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

/** @file src/film_state.h
 *  @brief The state of a Film.  This is separate from Film so that
 *  state can easily be copied and kept around for reference
 *  by long-running jobs.  This avoids the jobs getting confused
 *  by the user changing Film settings during their run.
 */

#ifndef DVDOMATIC_FILM_STATE_H
#define DVDOMATIC_FILM_STATE_H

#include <sstream>
#include <boost/filesystem.hpp>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#include "scaler.h"
#include "util.h"

class Format;
class ContentType;
class Filter;

/** The state of a Film.  This is separate from Film so that
 *  state can easily be copied and kept around for reference
 *  by long-running jobs.  This avoids the jobs getting confused
 *  by the user changing Film settings during their run.
 */

class FilmState
{
public:
	FilmState ()
		: guess_dcp_long_name (false)
		, dcp_content_type (0)
		, frames_per_second (0)
		, format (0)
		, left_crop (0)
		, right_crop (0)
		, top_crop (0)
		, bottom_crop (0)
		, scaler (Scaler::get_from_id ("bicubic"))
		, dcp_frames (0)
		, dcp_ab (false)
		, length (0)
		, audio_channels (0)
		, audio_sample_rate (0)
		, audio_sample_format (AV_SAMPLE_FMT_NONE)
	{}

	/** Given a file name, return its full path within the Film's directory */
	std::string file (std::string f) const {
		std::stringstream s;
		s << directory << "/" << f;
		return s.str ();
	}

	/** Given a directory name, return its full path within the Film's directory.
	 *  The directory (and its parents) will be created if they do not exist.
	 */
	std::string dir (std::string d) const {
		std::stringstream s;
		s << directory << "/" << d;
		boost::filesystem::create_directories (s.str ());
		return s.str ();
	}

	std::string thumb_file (int) const;
	int thumb_frame (int) const;
	
	void write_metadata (std::ofstream &) const;
	void read_metadata (std::string, std::string);

	/** Complete path to directory containing the film metadata;
	    must not be relative.
	*/
	std::string directory;
	/** Name for DVD-o-matic */
	std::string name;
	/** File or directory containing content (relative to our directory) */
	std::string content;
	/** DCP long name (e.g. BLUES-BROTHERS_FTR_F_EN-XX ...) */
	std::string dcp_long_name;
	/** true if we are guessing the dcp_long_name from other state */
	bool guess_dcp_long_name;
	/** The type of content that this Film represents (feature, trailer etc.) */
	ContentType const * dcp_content_type;
	/** Frames per second of the source */
	float frames_per_second;
	/** The format to present this Film in (flat, scope, etc.) */
	Format const * format;
	/** Number of pixels to crop from the left-hand side of the original picture */
	int left_crop;
	/** Number of pixels to crop from the right-hand side of the original picture */
	int right_crop;
	/** Number of pixels to crop from the top of the original picture */
	int top_crop;
	/** Number of pixels to crop from the bottom of the original picture */
	int bottom_crop;
	/** Video filters that should be used when generating DCPs */
	std::vector<Filter const *> filters;
	/** Scaler algorithm to use */
	Scaler const * scaler;
	/** Number of frames to put in the DCP, or 0 for all */
	int dcp_frames;
	/** true to create an A/B comparison DCP, where the left half of the image
	    is the video without any filters or post-processing, and the right half
	    has the specified filters and post-processing.
	*/
	bool dcp_ab;

	/* Data which is cached to speed things up */

	/** Vector of frame indices for each of our `thumbnails */
	std::vector<int> thumbs;
	/** Size, in pixels, of the source (ignoring cropping) */
	Size size;
	/** Length in frames */
	int length;
	/** Number of audio channels */
	int audio_channels;
	/** Sample rate of the audio, in Hz */
	int audio_sample_rate;
	/** Format of the audio samples */
	AVSampleFormat audio_sample_format;

private:
	std::string thumb_file_for_frame (int) const;
};

#endif
