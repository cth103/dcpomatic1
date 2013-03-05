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

#include "video_source.h"
#include "video_sink.h"

using boost::shared_ptr;
using boost::bind;

void
VideoSource::connect_video (shared_ptr<VideoSink> s)
{
	Video.connect (bind (&VideoSink::process_video, s, _1, _2, _3));
}

void
TimedVideoSource::connect_video (shared_ptr<TimedVideoSink> s)
{
	Video.connect (bind (&TimedVideoSink::process_video, s, _1, _2, _3, _4));
}
