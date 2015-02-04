/*
    Copyright (C) 2015 Carl Hetherington <cth@carlh.net>

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
#include "lib/ffmpeg_content.h"
#include "lib/ratio.h"

using std::list;
using std::string;
using std::cerr;
using std::stringstream;
using boost::shared_ptr;
using boost::optional;

static
void
test (libdcp::Size content_size, libdcp::Size display_size, libdcp::Size film_size, Crop crop, Ratio const * ratio, bool scale, libdcp::Size correct)
{
	shared_ptr<Film> film;
	stringstream s;
	s << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<Content>"
		"<Type>FFmpeg</Type>"
		"<Path>/home/c.hetherington/DCP/prophet_clip.mkv</Path>"
		"<Digest>f3f23663da5bef6d2cbaa0db066f3351314142710</Digest>"
		"<Position>0</Position>"
		"<TrimStart>0</TrimStart>"
		"<TrimEnd>0</TrimEnd>"
		"<VideoLength>2879</VideoLength>"
		"<VideoWidth>" << content_size.width << "</VideoWidth>"
		"<VideoHeight>" << content_size.height << "</VideoHeight>"
		"<VideoFrameRate>23.97602462768555</VideoFrameRate>"
		"<OriginalVideoFrameRate>23.97602462768555</OriginalVideoFrameRate>"
		"<VideoFrameType>0</VideoFrameType>"
		"<SampleAspectRatio>1</SampleAspectRatio>"
		"<LeftCrop>" << crop.left << "</LeftCrop>"
		"<RightCrop>" << crop.right << "</RightCrop>"
		"<TopCrop>" << crop.top << "</TopCrop>"
		"<BottomCrop>" << crop.bottom << "</BottomCrop>"
		"<Scale>";

	if (ratio) {
		s << "<Ratio>" << ratio->id() << "</Ratio>";
	} else {
		s << "<Scale>" << scale << "</Scale>";
	}

	s << "</Scale>"
		"<ColourConversion>"
		"<InputGamma>2.4</InputGamma>"
		"<InputGammaLinearised>1</InputGammaLinearised>"
		"<Matrix i=\"0\" j=\"0\">0.4124564</Matrix>"
		"<Matrix i=\"0\" j=\"1\">0.3575761</Matrix>"
		"<Matrix i=\"0\" j=\"2\">0.1804375</Matrix>"
		"<Matrix i=\"1\" j=\"0\">0.2126729</Matrix>"
		"<Matrix i=\"1\" j=\"1\">0.7151522</Matrix>"
		"<Matrix i=\"1\" j=\"2\">0.072175</Matrix>"
		"<Matrix i=\"2\" j=\"0\">0.0193339</Matrix>"
		"<Matrix i=\"2\" j=\"1\">0.119192</Matrix>"
		"<Matrix i=\"2\" j=\"2\">0.9503041</Matrix>"
		"<OutputGamma>2.6</OutputGamma>"
		"</ColourConversion>"
		"<AudioGain>0</AudioGain>"
		"<AudioDelay>0</AudioDelay>"
		"<SubtitleXOffset>0</SubtitleXOffset>"
		"<SubtitleYOffset>0</SubtitleYOffset>"
		"<SubtitleXScale>0</SubtitleXScale>"
		"<SubtitleYScale>0</SubtitleYScale>"
		"</Content>";

	shared_ptr<cxml::Document> doc (new cxml::Document ());
	doc->read_string(s.str ());

	list<string> notes;
	shared_ptr<VideoContent> vc (new FFmpegContent (film, doc, 10, notes));

	optional<VideoContentScale> sc;
	if (ratio) {
		sc = VideoContentScale (ratio);
	} else {
		sc = VideoContentScale (scale);
	}

	libdcp::Size answer = sc.get().size (vc, display_size, film_size);
	if (answer != correct) {
		cerr << answer.width << "x" << answer.height << " instead of " << correct.width << "x" << correct.height << "\n";
	}
	BOOST_CHECK (answer == correct);
}
      
/* Test scale and stretch to specified ratio */
BOOST_AUTO_TEST_CASE (video_content_scale_test_to_ratio)
{
	/* To DCP */

	// Flat in flat container
	test (
		libdcp::Size (400, 200),
		libdcp::Size (1998, 1080),
		libdcp::Size (1998, 1080),
		Crop (0, 0, 0, 0),
		Ratio::from_id ("185"),
		true,
		libdcp::Size (1998, 1080)
		);

	// Scope in flat container
	test (
		libdcp::Size (400, 200),
		libdcp::Size (1998, 1080),
		libdcp::Size (1998, 1080),
		Crop (0, 0, 0, 0),
		Ratio::from_id ("239"),
		true,
		libdcp::Size (1998, 836)
		);
	
	// Flat in scope container
	test (
		libdcp::Size (400, 200),
		libdcp::Size (2048, 858),
		libdcp::Size (2048, 858),
		Crop (0, 0, 0, 0),
		Ratio::from_id ("185"),
		true,
		libdcp::Size (1587, 858)
		);

	
	/* To player */

	// Flat in flat container
	test (
		libdcp::Size (400, 200),
		libdcp::Size (185, 100),
		libdcp::Size (1998, 1080),
		Crop (0, 0, 0, 0),
		Ratio::from_id ("185"),
		true,
		libdcp::Size (185, 100)
		);

	// Scope in flat container
	test (
		libdcp::Size (400, 200),
		libdcp::Size (185, 100),
		libdcp::Size (1998, 1080),
		Crop (0, 0, 0, 0),
		Ratio::from_id ("239"),
		true,
		libdcp::Size (185, 77)
		);
	
	// Flat in scope container
	test (
		libdcp::Size (400, 200),
		libdcp::Size (239, 100),
		libdcp::Size (2048, 858),
		Crop (0, 0, 0, 0),
		Ratio::from_id ("185"),
		true,
		libdcp::Size (185, 100)
		);
}

/* Test no scale */
BOOST_AUTO_TEST_CASE (video_content_scale_no_scale)
{
	/* No scale where the content is bigger than even the film container */
	test (
		libdcp::Size (1920, 1080),
		libdcp::Size (887, 371),
		libdcp::Size (2048, 858),
		Crop (),
		0,
		false,
		libdcp::Size (659, 371)
		);
}

