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

#include <libdcp/types.h>
#include "types.h"
#include "video_content.h"

class VideoExaminer
{
public:
	virtual ~VideoExaminer () {}
	virtual boost::optional<float> video_frame_rate () const = 0;
	virtual libdcp::Size video_size () const = 0;
	virtual VideoContent::Frame video_length () const = 0;
	virtual boost::optional<float> sample_aspect_ratio () const {
		return boost::optional<float> ();
	}
};
