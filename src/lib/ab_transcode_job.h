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

/** @file src/ab_transcode_job.h
 *  @brief Job to run a transcoder which produces output for A/B comparison of various settings.
 */

#include <boost/shared_ptr.hpp>
#include "job.h"
#include "options.h"

class Film;

/** @class ABTranscodeJob
 *  @brief Job to run a transcoder which produces output for A/B comparison of various settings.
 *
 *  The right half of the frame will be processed using the Film supplied;
 *  the left half will be processed using the same state but with the reference
 *  filters and scaler.
 */
class ABTranscodeJob : public Job
{
public:
	ABTranscodeJob (
		boost::shared_ptr<Film> f,
		DecodeOptions o
		);

	std::string name () const;
	void run ();

private:
	DecodeOptions _decode_opt;
	
	/** Copy of our Film using the reference filters and scaler */
	boost::shared_ptr<Film> _film_b;
};