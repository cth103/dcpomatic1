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

#ifndef DCPOMATIC_TIMELINE_VIEW_H
#define DCPOMATIC_TIMELINE_VIEW_H

#include <boost/noncopyable.hpp>
#include "lib/types.h"
#include "lib/rect.h"

class wxGraphicsContext;

class Timeline;

/** Parent class for components of the timeline (e.g. a piece of content or an axis) */
class TimelineView : public boost::noncopyable
{
public:
	TimelineView (Timeline& t);
	virtual ~TimelineView () {}
		
	void paint (wxGraphicsContext* g);
	void force_redraw ();

	virtual dcpomatic::Rect<int> bbox () const = 0;
	
protected:
	int time_x (Time t) const;
	Timeline& _timeline;

private:
	virtual void do_paint (wxGraphicsContext *) = 0;
	
	dcpomatic::Rect<int> _last_paint_bbox;
};

#endif
