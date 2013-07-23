/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

#ifndef DCPOMATIC_VIDEO_CONTENT_H
#define DCPOMATIC_VIDEO_CONTENT_H

#include "content.h"

class VideoExaminer;
class Ratio;

class VideoContentProperty
{
public:
	static int const VIDEO_SIZE;
	static int const VIDEO_FRAME_RATE;
	static int const VIDEO_FRAME_TYPE;
	static int const VIDEO_CROP;
	static int const VIDEO_RATIO;
};

class VideoContent : public virtual Content
{
public:
	typedef int Frame;

	VideoContent (boost::shared_ptr<const Film>, Time, VideoContent::Frame);
	VideoContent (boost::shared_ptr<const Film>, boost::filesystem::path);
	VideoContent (boost::shared_ptr<const Film>, boost::shared_ptr<const cxml::Node>);

	void as_xml (xmlpp::Node *) const;
	std::string technical_summary () const;
	virtual std::string information () const;
	virtual std::string identifier () const;

	VideoContent::Frame video_length () const {
		boost::mutex::scoped_lock lm (_mutex);
		return _video_length;
	}

	libdcp::Size video_size () const {
		boost::mutex::scoped_lock lm (_mutex);
		return _video_size;
	}
	
	float video_frame_rate () const {
		boost::mutex::scoped_lock lm (_mutex);
		return _video_frame_rate;
	}

	void set_video_frame_type (VideoFrameType);

	void set_left_crop (int);
	void set_right_crop (int);
	void set_top_crop (int);
	void set_bottom_crop (int);

	VideoFrameType video_frame_type () const {
		boost::mutex::scoped_lock lm (_mutex);
		return _video_frame_type;
	}

	Crop crop () const {
		boost::mutex::scoped_lock lm (_mutex);
		return _crop;
	}

	void set_ratio (Ratio const *);

	Ratio const * ratio () const {
		boost::mutex::scoped_lock lm (_mutex);
		return _ratio;
	}

protected:
	void take_from_video_examiner (boost::shared_ptr<VideoExaminer>);

	VideoContent::Frame _video_length;

private:
	friend class ffmpeg_pts_offset_test;
	friend class best_dcp_frame_rate_test_single;
	friend class best_dcp_frame_rate_test_double;
	friend class audio_sampling_rate_test;
	
	libdcp::Size _video_size;
	float _video_frame_rate;
	VideoFrameType _video_frame_type;
	Crop _crop;
	Ratio const * _ratio;
};

#endif
