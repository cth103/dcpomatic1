/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

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

#include "video_examiner.h"

namespace Magick {
	class Image;
}

class ImageContent;

class ImageExaminer : public VideoExaminer
{
public:
	ImageExaminer (boost::shared_ptr<const Film>, boost::shared_ptr<const ImageContent>, boost::shared_ptr<Job>);

	boost::optional<float> video_frame_rate () const;
	libdcp::Size video_size () const;
	VideoContent::Frame video_length () const {
		return _video_length;
	}

private:
	boost::weak_ptr<const Film> _film;
	boost::shared_ptr<const ImageContent> _image_content;
	boost::optional<libdcp::Size> _video_size;
	VideoContent::Frame _video_length;
};
