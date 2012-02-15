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

#include <sstream>
#include <iomanip>
extern "C" {
#include <libavcodec/avcodec.h>
}

class Filter;

/** Parameters for a transcoding operation */
class Parameters
{
public:
	Parameters (std::string const & v, std::string const & e, std::string const & a)
		: out_width (0)
		, out_height (0)
		, apply_crop (true)
		, num_frames (0)
		, decode_video (true)
		, decode_video_frequency (0)
		, decode_audio (true)
		, audio_channels (0)
		, audio_sample_rate (0)
		, audio_sample_format (AV_SAMPLE_FMT_NONE)
		, frames_per_second (0)
		, left_crop (0)
		, right_crop (0)
		, top_crop (0)
		, bottom_crop (0)
		, _video_out_path (v)
		, _video_out_extension (e)
		, _audio_out_path (a)
	{}

	std::string video_out_path (int f, bool t) const {
		std::stringstream s;
		s << _video_out_path << "/";
		s.width (8);
		s << std::setfill('0') << f << _video_out_extension;

		if (t) {
			s << ".tmp";
		}

		return s.str ();
	}

	std::string audio_out_path (int c, bool t) const {
		std::stringstream s;
		s << _audio_out_path << "/" << (c + 1) << ".wav";
		if (t) {
			s << ".tmp";
		}

		return s.str ();
	}
	
	int out_width;              ///< width of output images
	int out_height;             ///< height of output images
	bool apply_crop;            ///< true to apply cropping
	int num_frames;             ///< number of video frames to decode, or 0 for all
	bool decode_video;          ///< true to decode video, otherwise false
	int decode_video_frequency; ///< skip frames so that this many are decoded in all (or 0)
	                            ///< (useful for generating `thumbnails' of a video)
	bool decode_audio;          ///< true to decode audio, otherwise false
	int audio_channels;
	int audio_sample_rate;
	int audio_sample_format;
	int frames_per_second;
	std::string content;
	int left_crop;
	int right_crop;
	int top_crop;
	int bottom_crop;
	std::vector<Filter const *> filters;

private:
	std::string _video_out_path;
	std::string _video_out_extension;
	std::string _audio_out_path;
};
