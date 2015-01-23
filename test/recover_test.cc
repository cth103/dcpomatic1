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

#include <boost/test/unit_test.hpp>
#include <libdcp/stereo_picture_asset.h>
#include "lib/film.h"
#include "lib/dcp_content_type.h"
#include "lib/image_content.h"
#include "lib/ratio.h"
#include "test.h"

using std::cout;
using std::string;
using boost::shared_ptr;

static void
note (libdcp::NoteType, string n)
{
	cout << n << "\n";
}

/** Test recovery of a DCP transcode after a crash */
BOOST_AUTO_TEST_CASE (recover_test)
{
	shared_ptr<Film> film = new_test_film ("recover_test");
	film->set_dcp_content_type (DCPContentType::from_isdcf_name ("FTR"));
	film->set_container (Ratio::from_id ("185"));
	film->set_name ("recover_test");
	film->set_three_d (true);

	shared_ptr<ImageContent> content (new ImageContent (film, "test/data/3d_test"));
	content->set_video_frame_type (VIDEO_FRAME_TYPE_3D_LEFT_RIGHT);
	content->set_video_frame_rate (24);
	film->examine_and_add_content (content);
	wait_for_jobs ();

	film->make_dcp ();
	wait_for_jobs ();

	boost::filesystem::copy_file (
		"build/test/recover_test/video/185_2K_991ced0f8e6f0ba3df2f6606a352ec99_24_bicubic_100000000_P_S_3D.mxf",
		"build/test/recover_test/original.mxf"
		);
	
	boost::filesystem::resize_file ("build/test/recover_test/video/185_2K_991ced0f8e6f0ba3df2f6606a352ec99_24_bicubic_100000000_P_S_3D.mxf", 2 * 1024 * 1024);

	film->make_dcp ();
	wait_for_jobs ();

	shared_ptr<libdcp::StereoPictureAsset> A (new libdcp::StereoPictureAsset ("build/test/recover_test", "original.mxf"));
	shared_ptr<libdcp::StereoPictureAsset> B (new libdcp::StereoPictureAsset ("build/test/recover_test/video", "185_2K_991ced0f8e6f0ba3df2f6606a352ec99_24_bicubic_100000000_P_S_3D.mxf"));

	libdcp::EqualityOptions eq;
	eq.mxf_names_can_differ = true;
	BOOST_CHECK (A->equals (B, eq, boost::bind (&note, _1, _2)));
}
