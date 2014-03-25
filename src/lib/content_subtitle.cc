/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#include "content_subtitle.h"

ContentTime
ContentTextSubtitle::from () const
{
	/* XXX: assuming we have some subs and they are all at the same time */
	assert (!subs.empty ());
	return ContentTime::from_seconds (double (subs.front().in().to_ticks()) / 250);
}

ContentTime
ContentTextSubtitle::to () const
{
	/* XXX: assuming we have some subs and they are all at the same time */
	assert (!subs.empty ());
	return ContentTime::from_seconds (double (subs.front().out().to_ticks()) / 250);
}