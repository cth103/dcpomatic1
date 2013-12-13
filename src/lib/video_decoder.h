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

#ifndef DCPOMATIC_VIDEO_DECODER_H
#define DCPOMATIC_VIDEO_DECODER_H

#include <boost/signals2.hpp>
#include <boost/shared_ptr.hpp>
#include "decoder.h"
#include "video_content.h"
#include "util.h"

class VideoContent;
class Image;

class VideoDecoder : public virtual Decoder
{
public:
	VideoDecoder (boost::shared_ptr<const Film>, boost::shared_ptr<const VideoContent>);

	/** Emitted when a video frame is ready.
	 *  First parameter is the video image.
	 *  Second parameter is the eye(s) which should see this image.
	 *  Third parameter is true if the image is the same as the last one that was emitted for this Eyes value.
	 *  Fourth parameter is the time within our source.
	 */
	boost::signals2::signal<void (boost::shared_ptr<const Image>, Eyes, bool, ContentTime)> Video;
	
protected:

	void video (boost::shared_ptr<const Image>, bool, ContentTime);
	boost::shared_ptr<const VideoContent> _video_content;
};

#endif
