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

#include <libdcp/sound_frame.h>
#include <libdcp/cpl.h>
#include <libdcp/reel.h>
#include <libdcp/sound_asset.h>
#include "sndfile_content.h"

using boost::lexical_cast;

static
void test_audio_delay (int delay_in_ms)
{
	string const film_name = "audio_delay_test_" + lexical_cast<string> (delay_in_ms);
	shared_ptr<Film> film = new_test_film (film_name);
	film->set_dcp_content_type (DCPContentType::from_dci_name ("FTR"));
	film->set_container (Ratio::from_id ("185"));
	film->set_name (film_name);

	shared_ptr<SndfileContent> content (new SndfileContent (film, "test/data/staircase.wav"));
	content->set_audio_delay (delay_in_ms);
	film->examine_and_add_content (content);
	wait_for_jobs ();

	film->make_dcp ();
	wait_for_jobs ();

	boost::filesystem::path path = "build/test";
	path /= film_name;
	path /= film->dcp_name ();
	libdcp::DCP check (path.string ());
	check.read ();

	shared_ptr<const libdcp::SoundAsset> sound_asset = check.cpls().front()->reels().front()->main_sound ();
	BOOST_CHECK (sound_asset);

	/* Sample index in the DCP */
	int n = 0;
	/* DCP sound asset frame */
	int frame = 0;
	/* Delay in frames */
	int const delay_in_frames = delay_in_ms * 48000 / 1000;
	bool done = false;

	while (n < sound_asset->intrinsic_duration()) {
		shared_ptr<const libdcp::SoundFrame> sound_frame = sound_asset->get_frame (frame++);
		uint8_t const * d = sound_frame->data ();
		
		for (int i = 0; i < sound_frame->size(); i += (3 * sound_asset->channels())) {

			/* Mono input so it will appear on centre */
			int const sample = d[i + 7] | (d[i + 8] << 8);

			int delayed = n - delay_in_frames;
			if (delayed < 0 || delayed >= 4800) {
				delayed = 0;
			}

			BOOST_CHECK_EQUAL (sample, delayed);
			++n;
		}
	}
}


/* Test audio delay when specified in a piece of audio content */
BOOST_AUTO_TEST_CASE (audio_delay_test)
{
	test_audio_delay (0);
	test_audio_delay (42);
	test_audio_delay (-66);
}
