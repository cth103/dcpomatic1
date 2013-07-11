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

#include "video_decoder.h"
#include "image.h"

#include "i18n.h"

using std::cout;
using boost::shared_ptr;

VideoDecoder::VideoDecoder (shared_ptr<const Film> f)
	: Decoder (f)
	, _video_position (0)
{

}

void
VideoDecoder::video (shared_ptr<const Image> image, bool same, VideoContent::Frame frame)
{
        Video (image, same, frame);
	_video_position = frame + 1;
}

