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
#include <boost/filesystem.hpp>
#include "lib/ffmpeg_content.h"
#include "lib/ffmpeg_decoder.h"
#include "lib/log.h"
#include "lib/film.h"
#include "test.h"

using std::cout;
using std::cerr;
using boost::shared_ptr;
using boost::optional;

/** @param black Frame index of first frame in the video */ 
static void
test (boost::filesystem::path file, float fps, int first)
{
	boost::filesystem::path path = private_data / file;
	if (!boost::filesystem::exists (path)) {
		cerr << "Skipping test: " << path.string() << " not found.\n";
		return;
	}

	shared_ptr<Film> film = new_test_film ("ffmpeg_decoder_seek_test_" + file.string());
	shared_ptr<FFmpegContent> content (new FFmpegContent (film, path)); 
	film->examine_and_add_content (content);
	wait_for_jobs ();
	shared_ptr<Log> log (new NullLog);
	FFmpegDecoder decoder (content, log);

	BOOST_CHECK_CLOSE (decoder.video_content()->video_frame_rate(), fps, 0.01);
	
	VideoFrame const N = decoder.video_content()->video_length().frames (decoder.video_content()->video_frame_rate ());
	decoder.test_gaps = 0;
	for (VideoFrame i = 0; i < N; ++i) {
		optional<ContentVideo> v;
		v = decoder.get_video (i, true);
		if (i < first) {
			BOOST_CHECK (!v);
		} else {
			BOOST_CHECK (v);
			BOOST_CHECK_EQUAL (v->frame, i);
		}
	}
	BOOST_CHECK_EQUAL (decoder.test_gaps, 0);
}

/** Check that the FFmpeg decoder produces sequential frames without gaps or dropped frames;
 *  (dropped frames being checked by assert() in VideoDecoder).  Also that the decoder picks up frame rates correctly.
 */
BOOST_AUTO_TEST_CASE (ffmpeg_decoder_sequential_test)
{
	//test ("boon_telly.mkv", 29.97, 0);
	//test ("Sintel_Trailer1.480p.DivX_Plus_HD.mkv", 24, 0);
	test ("prophet_clip.mkv", 23.976, 12);
}

