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

#include <iostream>
#include <boost/lexical_cast.hpp>
#include <Magick++.h>
#include "moving_image_content.h"
#include "moving_image_examiner.h"
#include "film.h"
#include "job.h"
#include "exceptions.h"

#include "i18n.h"

using std::cout;
using std::list;
using std::sort;
using boost::shared_ptr;
using boost::lexical_cast;

MovingImageExaminer::MovingImageExaminer (shared_ptr<const Film> film, shared_ptr<const MovingImageContent> content, shared_ptr<Job> job)
	: MovingImage (content)
	, _film (film)
	, _video_length (0)
{
	list<unsigned int> frames;
	unsigned int files = 0;
	
	for (boost::filesystem::directory_iterator i(content->path()); i != boost::filesystem::directory_iterator(); ++i) {
		if (boost::filesystem::is_regular_file (i->path ())) {
			++files;
		}
	}

	int j = 0;
	for (boost::filesystem::directory_iterator i(content->path()); i != boost::filesystem::directory_iterator(); ++i) {
		if (!boost::filesystem::is_regular_file (i->path ())) {
			continue;
		}

		if (valid_image_file (i->path ())) {
			int n = lexical_cast<int> (i->path().stem().string());
			frames.push_back (n);
			_files.push_back (i->path().filename ());

			if (!_video_size) {
				using namespace MagickCore;
				Magick::Image* image = new Magick::Image (i->path().string());
				_video_size = libdcp::Size (image->columns(), image->rows());
				delete image;
			}
		}

		job->set_progress (float (j) / files);
		++j;
	}

	frames.sort ();
	sort (_files.begin(), _files.end ());
	
	if (frames.size() < 2) {
		throw StringError (String::compose (_("only %1 file(s) found in moving image directory"), frames.size ()));
	}

	if (frames.front() != 0 && frames.front() != 1) {
		throw StringError (String::compose (_("first frame in moving image directory is number %1"), frames.front ()));
	}

	if (frames.back() != frames.size() && frames.back() != (frames.size() - 1)) {
		throw StringError (String::compose (_("there are %1 images in the directory but the last one is number %2"), frames.size(), frames.back ()));
	}

	_video_length = frames.size ();
}

libdcp::Size
MovingImageExaminer::video_size () const
{
	return _video_size.get ();
}

int
MovingImageExaminer::video_length () const
{
	cout << "ex video length is " << _video_length << "\n";
	return _video_length;
}

float
MovingImageExaminer::video_frame_rate () const
{
	return 24;
}

