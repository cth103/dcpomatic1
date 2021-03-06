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

#include <iostream>
#include <Magick++.h>
#include "image_content.h"
#include "image_examiner.h"
#include "film.h"
#include "job.h"
#include "exceptions.h"
#include "config.h"

#include "i18n.h"

using std::cout;
using std::list;
using std::sort;
using boost::shared_ptr;
using boost::optional;

ImageExaminer::ImageExaminer (shared_ptr<const Film> film, shared_ptr<const ImageContent> content, shared_ptr<Job>)
	: _film (film)
	, _image_content (content)
	, _video_length (0)
{
#ifdef DCPOMATIC_IMAGE_MAGICK
	using namespace MagickCore;
#endif
	Magick::Image* image = new Magick::Image (content->path(0).string());
	_video_size = libdcp::Size (image->columns(), image->rows());
	delete image;

	if (content->still ()) {
		_video_length = Config::instance()->default_still_length() * video_frame_rate().get_value_or (24);
	} else {
		_video_length = _image_content->number_of_paths ();
	}
}

libdcp::Size
ImageExaminer::video_size () const
{
	return _video_size.get ();
}

optional<float>
ImageExaminer::video_frame_rate () const
{
	/* Don't know */
	return optional<float> ();
}
