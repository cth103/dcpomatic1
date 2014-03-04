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

#include <boost/test/unit_test.hpp>
#include "lib/image_content.h"
#include "lib/dcp_content_type.h"
#include "lib/film.h"
#include "lib/ratio.h"
#include "test.h"

/** @file test/black_fill_test.cc
 *  @brief Test insertion of black frames between video content.
 */

using boost::shared_ptr;

BOOST_AUTO_TEST_CASE (black_fill_test)
{
	shared_ptr<Film> film = new_test_film ("black_fill_test");
	film->set_dcp_content_type (DCPContentType::from_dci_name ("FTR"));
	film->set_name ("black_fill_test");
	film->set_container (Ratio::from_id ("185"));
	film->set_sequence_video (false);
	shared_ptr<ImageContent> contentA (new ImageContent (film, "test/data/simple_testcard_640x480.png"));
	contentA->set_scale (VideoContentScale (Ratio::from_id ("185")));
	shared_ptr<ImageContent> contentB (new ImageContent (film, "test/data/simple_testcard_640x480.png"));
	contentB->set_scale (VideoContentScale (Ratio::from_id ("185")));

	film->examine_and_add_content (contentA);
	film->examine_and_add_content (contentB);
	wait_for_jobs ();

	contentA->set_video_length (3);
	contentA->set_position (film->video_frames_to_time (2));
	contentB->set_video_length (1);
	contentB->set_position (film->video_frames_to_time (7));

	film->make_dcp ();

	wait_for_jobs ();

	boost::filesystem::path ref;
	ref = "test";
	ref /= "data";
	ref /= "black_fill_test";

	boost::filesystem::path check;
	check = "build";
	check /= "test";
	check /= "black_fill_test";
	check /= film->dcp_name();

	check_dcp (ref.string(), check.string());
}

