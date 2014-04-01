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

/** @file  test/seek_zero_test.cc
 *  @brief Test seek to zero with a raw FFmpegDecoder (without the player
 *  confusing things as it might in ffmpeg_seek_test).
 */

#include <boost/test/unit_test.hpp>
#include "lib/film.h"
#include "lib/ffmpeg_content.h"
#include "lib/ratio.h"
#include "lib/dcp_content_type.h"
#include "lib/ffmpeg_decoder.h"
#include "lib/content_video.h"
#include "test.h"

using std::cout;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using boost::optional;

BOOST_AUTO_TEST_CASE (seek_zero_test)
{
	shared_ptr<Film> film = new_test_film ("seek_zero_test");
	film->set_name ("seek_zero_test");
	film->set_container (Ratio::from_id ("185"));
	film->set_dcp_content_type (DCPContentType::from_pretty_name ("Test"));
	shared_ptr<FFmpegContent> content (new FFmpegContent (film, "test/data/count300bd48.m2ts"));
	content->set_scale (VideoContentScale (Ratio::from_id ("185")));
	film->examine_and_add_content (content);
	wait_for_jobs ();

	FFmpegDecoder decoder (content, film->log());
	optional<ContentVideo> a = decoder.get_video (0, true);
	optional<ContentVideo> b = decoder.get_video (0, true);
	BOOST_CHECK_EQUAL (a->frame, 0);
	BOOST_CHECK_EQUAL (b->frame, 0);
}