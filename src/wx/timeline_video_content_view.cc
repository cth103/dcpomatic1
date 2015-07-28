/*
    Copyright (C) 2013-2015 Carl Hetherington <cth@carlh.net>

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

#include "lib/ffmpeg_content.h"
#include "timeline_video_content_view.h"

using boost::dynamic_pointer_cast;
using boost::shared_ptr;

TimelineVideoContentView::TimelineVideoContentView (Timeline& tl, shared_ptr<Content> c)
	: TimelineContentView (tl, c)
{

}

wxString
TimelineVideoContentView::type () const
{
	if (dynamic_pointer_cast<FFmpegContent> (content ())) {
		return _("video");
	} else {
		return _("still");
	}
}

wxColour
TimelineVideoContentView::colour () const
{
	return wxColour (242, 92, 120, 255);
}
