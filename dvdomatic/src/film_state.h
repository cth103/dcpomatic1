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

#ifndef DVDOMATIC_FILM_STATE_H
#define DVDOMATIC_FILM_STATE_H

#include <boost/filesystem.hpp>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#include "scaler.h"

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
		, format (0)
		, left_crop (0)
		, right_crop (0)
		, top_crop (0)
		, bottom_crop (0)
		, scaler (Scaler::get_from_id ("bicubic"))
		, dcp_frames (0)
		, dcp_ab (false)
		, width (0)
		, height (0)
		, length (0)
		, frames_per_second (0)
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

	/** Complete path to directory containing the film metadata;
	    must not be relative.
	*/
	std::string directory;
	/** Name for DVD-o-matic */
	std::string name;
	/** File containing content (relative to directory) */
	std::string content;
	/** DCP long name (e.g. BLUES-BROTHERS_FTR_F_EN-XX ...) */
	std::string dcp_long_name;
	bool guess_dcp_long_name;
	/** The type of content that this Film represents (feature, trailer etc.) */
	ContentType const * dcp_content_type;
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
	std::vector<Filter const *> filters;
	/** Scaler algorithm to use.
	 *  (SWS_BICUBIC, SWS_X, SWS_AREA, SWS_GAUSS, SWS_LANCZOS, SWS_SINC, SWS_SPLINE, SWS_BILINEAR, SWS_FAST_BILINEAR)
	 */
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
	/** Width, in pixels, of the source (ignoring cropping) */
	int width;
	/** Height, in pixels, of the source (ignoring cropping) */
	int height;
	/** Length in frames */
	int length;
	/** Frames per second of the source */
	float frames_per_second;
	/** Number of audio channels */
	int audio_channels;
	/** Sample rate of the audio, in Hz */
	int audio_sample_rate;
	/** Format of the audio samples */
	AVSampleFormat audio_sample_format;
};

#endif
