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

#include <boost/signals2.hpp>
#include "decoder.h"
#include "rect.h"
#include "types.h"

class Film;
class TimedSubtitle;
class Image;

class SubtitleDecoder : public virtual Decoder
{
public:
	SubtitleDecoder (boost::shared_ptr<const Film>);

	boost::signals2::signal<void (boost::shared_ptr<Image>, dcpomatic::Rect<double>, Time, Time)> Subtitle;

protected:
	void subtitle (boost::shared_ptr<Image>, dcpomatic::Rect<double>, Time, Time);
};