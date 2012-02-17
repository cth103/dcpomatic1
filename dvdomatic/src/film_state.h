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

class Film;

class FilmState
{
public:
	FilmState ()
		: _dcp_content_type (0)
		, _format (0)
		, _left_crop (0)
		, _right_crop (0)
		, _top_crop (0)
		, _bottom_crop (0)
		, _dcp_frames (0)
		, _dcp_ab (false)
		, _width (0)
		, _height (0)
		, _length (0)
		, _frames_per_second (0)
		, _audio_channels (0)
		, _audio_sample_rate (0)
		, _audio_sample_format (AV_SAMPLE_FMT_NONE)
	{}

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
