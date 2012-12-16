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

/** @file  src/decoder.h
 *  @brief Parent class for decoders of content.
 */

#ifndef DVDOMATIC_DECODER_H
#define DVDOMATIC_DECODER_H

#include <vector>
#include <string>
#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>
#include "util.h"
#include "stream.h"
#include "video_source.h"
#include "audio_source.h"

class Job;
class Options;
class Image;
class Log;
class DelayLine;
class TimedSubtitle;
class Subtitle;
class Film;
class FilterGraph;

/** @class Decoder.
 *  @brief Parent class for decoders of content.
 *
 *  These classes can be instructed run through their content (by
 *  calling ::go), and they emit signals when video or audio data is
 *  ready for something else to process.
 */
class Decoder
{
public:
	Decoder (boost::shared_ptr<Film>, boost::shared_ptr<const Options>, Job *);
	virtual ~Decoder () {}

	virtual bool pass () = 0;
	virtual void seek (SourceFrame);

protected:
	/** our Film */
	boost::shared_ptr<Film> _film;
	/** our options */
	boost::shared_ptr<const Options> _opt;
	/** associated Job, or 0 */
	Job* _job;
};

#endif
