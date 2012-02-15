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

/** Parameters for a transcoding operation */
class Parameters
{
public:
	Parameters ()
		: out_width (0)
		, out_height (0)
		, apply_crop (true)
		, num_frames (0)
		, decode_video (true)
		, decode_video_frequency (0)
		, decode_audio (true)
	{}
	
	int out_width;              ///< width of output images
	int out_height;             ///< height of output images
	bool apply_crop;            ///< true to apply cropping
	int num_frames;             ///< number of video frames to decode, or 0 for all
	bool decode_video;          ///< true to decode video, otherwise false
	int decode_video_frequency; ///< skip frames so that this many are decoded in all (or 0)
	                            ///< (useful for generating `thumbnails' of a video)
	bool decode_audio;          ///< true to decode audio, otherwise false
};
