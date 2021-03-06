/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#include "video_decoder.h"
#include "image.h"

#include "i18n.h"

using std::cout;
using boost::shared_ptr;

VideoDecoder::VideoDecoder (shared_ptr<const Film> f, shared_ptr<const VideoContent> c)
	: Decoder (f)
	, _video_content (c)
	, _video_position (0)
{

}

void
VideoDecoder::video (shared_ptr<const ImageProxy> image, bool same, VideoContent::Frame frame)
{
	switch (_video_content->video_frame_type ()) {
	case VIDEO_FRAME_TYPE_2D:
		Video (image, EYES_BOTH, PART_WHOLE, same, frame);
		break;
	case VIDEO_FRAME_TYPE_3D_ALTERNATE:
		Video (image, (frame % 2) ? EYES_RIGHT : EYES_LEFT, PART_WHOLE, same, frame / 2);
		break;
	case VIDEO_FRAME_TYPE_3D_LEFT_RIGHT:
		Video (image, EYES_LEFT, PART_LEFT_HALF, same, frame);
		Video (image, EYES_RIGHT, PART_RIGHT_HALF, same, frame);
		break;
	case VIDEO_FRAME_TYPE_3D_TOP_BOTTOM:
		Video (image, EYES_LEFT, PART_TOP_HALF, same, frame);
		Video (image, EYES_RIGHT, PART_BOTTOM_HALF, same, frame);
		break;
	case VIDEO_FRAME_TYPE_3D_LEFT:
		Video (image, EYES_LEFT, PART_WHOLE, same, frame);
		break;
	case VIDEO_FRAME_TYPE_3D_RIGHT:
		Video (image, EYES_RIGHT, PART_WHOLE, same, frame);
		break;
	}

	_video_position = frame + 1;
}

