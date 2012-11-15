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

/** @file src/ab_transcoder.h
 *  @brief A transcoder which uses one Film for the left half of the screen, and a different one
 *  for the right half (to facilitate A/B comparisons of settings)
 */

#include <boost/shared_ptr.hpp>
#include <stdint.h>
#include "util.h"

class Job;
class Encoder;
class VideoDecoder;
class AudioDecoder;
class Options;
class Image;
class Log;
class Subtitle;
class Film;

/** @class ABTranscoder
 *  @brief A transcoder which uses one Film for the left half of the screen, and a different one
 *  for the right half (to facilitate A/B comparisons of settings)
 */
class ABTranscoder
{
public:
	ABTranscoder (
		boost::shared_ptr<Film> a,
		boost::shared_ptr<Film> b,
		boost::shared_ptr<const Options> o,
		Job* j,
		boost::shared_ptr<Encoder> e
		);
	
	~ABTranscoder ();

	void go ();

private:
	void process_video (boost::shared_ptr<Image>, boost::shared_ptr<Subtitle>, int);
	
	boost::shared_ptr<Film> _film_a;
	boost::shared_ptr<Film> _film_b;
	boost::shared_ptr<const Options> _opt;
	Job* _job;
	boost::shared_ptr<Encoder> _encoder;
	std::pair<boost::shared_ptr<VideoDecoder>, boost::shared_ptr<AudioDecoder> > _da;
	std::pair<boost::shared_ptr<VideoDecoder>, boost::shared_ptr<AudioDecoder> > _db;
	boost::shared_ptr<Image> _image;
};
