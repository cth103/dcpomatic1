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

#include <string>
#include <iomanip>
#include <sstream>
#include "util.h"

/** Options for a transcoding operation */
class Options
{
public:

	Options (std::string f, std::string e, std::string m)
		: apply_crop (true)
		, num_frames (0)
		, decode_video (true)
		, decode_video_frequency (0)
		, decode_audio (true)
		, _frame_out_path (f)
		, _frame_out_extension (e)
		, _multichannel_audio_out_path (m)
	{}

	std::string frame_out_path () const {
		return _frame_out_path;
	}

	std::string frame_out_path (int f, bool t) const {
		std::stringstream s;
		s << _frame_out_path << "/";
		s.width (8);
		s << std::setfill('0') << f << _frame_out_extension;

		if (t) {
			s << ".tmp";
		}

		return s.str ();
	}

	std::string multichannel_audio_out_path () const {
		return _multichannel_audio_out_path;
	}

	std::string multichannel_audio_out_path (int c, bool t) const {
		std::stringstream s;
		s << _multichannel_audio_out_path << "/" << (c + 1) << ".wav";
		if (t) {
			s << ".tmp";
		}

		return s.str ();
	}

	Size out_size;              ///< size of output images
	bool apply_crop;            ///< true to apply cropping
	int num_frames;             ///< number of video frames to decode, or 0 for all
	bool decode_video;          ///< true to decode video, otherwise false
	int decode_video_frequency; ///< skip frames so that this many are decoded in all (or 0) (for generating thumbnails)
	bool decode_audio;          ///< true to decode audio, otherwise false

private:
	std::string _frame_out_path;
	std::string _frame_out_extension;
	std::string _multichannel_audio_out_path;
};
