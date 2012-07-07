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

/** @file src/film_state.cc
 *  @brief The state of a Film.  This is separate from Film so that
 *  state can easily be copied and kept around for reference
 *  by long-running jobs.  This avoids the jobs getting confused
 *  by the user changing Film settings during their run.
 */

#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>
#include <boost/filesystem.hpp>
#include "film_state.h"
#include "scaler.h"
#include "filter.h"
#include "format.h"
#include "dcp_content_type.h"
#include "util.h"

using namespace std;
using namespace boost;

/** Write state to a stream.
 *  @param f Stream to write to.
 */
void
FilmState::write_metadata (ofstream& f) const
{
	/* User stuff */
	f << "name " << name << "\n";
	f << "content " << content << "\n";
	f << "dcp_long_name " << dcp_long_name << "\n";
	f << "guess_dcp_long_name " << (guess_dcp_long_name ? "1" : "0") << "\n";
	if (dcp_content_type) {
		f << "dcp_content_type " << dcp_content_type->pretty_name () << "\n";
	}
	f << "frames_per_second " << frames_per_second << "\n";
	if (format) {
		f << "format " << format->as_metadata () << "\n";
	}
	f << "left_crop " << left_crop << "\n";
	f << "right_crop " << right_crop << "\n";
	f << "top_crop " << top_crop << "\n";
	f << "bottom_crop " << bottom_crop << "\n";
	for (vector<Filter const *>::const_iterator i = filters.begin(); i != filters.end(); ++i) {
		f << "filter " << (*i)->id () << "\n";
	}
	f << "scaler " << scaler->id () << "\n";
	f << "dcp_frames " << dcp_frames << "\n";
	f << "dcp_ab " << (dcp_ab ? "1" : "0") << "\n";
	f << "audio_gain " << audio_gain << "\n";
	f << "audio_delay " << audio_delay << "\n";
	f << "still_duration " << still_duration << "\n";

	/* Cached stuff; this is information about our content; we could
	   look it up each time, but that's slow.
	*/
	for (vector<int>::const_iterator i = thumbs.begin(); i != thumbs.end(); ++i) {
		f << "thumb " << *i << "\n";
	}
	f << "width " << size.width << "\n";
	f << "height " << size.height << "\n";
	f << "length " << length << "\n";
	f << "audio_channels " << audio_channels << "\n";
	f << "audio_sample_rate " << audio_sample_rate << "\n";
	f << "audio_sample_format " << audio_sample_format_to_string (audio_sample_format) << "\n";
}

/** Read state from a key / value pair.
 *  @param k Key.
 *  @param v Value.
 */
void
FilmState::read_metadata (string k, string v)
{
	/* User-specified stuff */
	if (k == "name") {
		name = v;
	} else if (k == "content") {
		content = v;
	} else if (k == "dcp_long_name") {
		dcp_long_name = v;
	} else if (k == "guess_dcp_long_name") {
		guess_dcp_long_name = (v == "1");
	} else if (k == "dcp_content_type") {
		dcp_content_type = DCPContentType::from_pretty_name (v);
	} else if (k == "frames_per_second") {
		frames_per_second = atof (v.c_str ());
	} else if (k == "format") {
		format = Format::from_metadata (v);
	} else if (k == "left_crop") {
		left_crop = atoi (v.c_str ());
	} else if (k == "right_crop") {
		right_crop = atoi (v.c_str ());
	} else if (k == "top_crop") {
		top_crop = atoi (v.c_str ());
	} else if (k == "bottom_crop") {
		bottom_crop = atoi (v.c_str ());
	} else if (k == "filter") {
		filters.push_back (Filter::from_id (v));
	} else if (k == "scaler") {
		scaler = Scaler::from_id (v);
	} else if (k == "dcp_frames") {
		dcp_frames = atoi (v.c_str ());
	} else if (k == "dcp_ab") {
		dcp_ab = (v == "1");
	} else if (k == "audio_gain") {
		audio_gain = atof (v.c_str ());
	} else if (k == "audio_delay") {
		audio_delay = atoi (v.c_str ());
	} else if (k == "still_duration") {
		still_duration = atoi (v.c_str ());
	}
	
	/* Cached stuff */
	if (k == "thumb") {
		int const n = atoi (v.c_str ());
		/* Only add it to the list if it still exists */
		if (filesystem::exists (thumb_file_for_frame (n))) {
			thumbs.push_back (n);
		}
	} else if (k == "width") {
		size.width = atoi (v.c_str ());
	} else if (k == "height") {
		size.height = atoi (v.c_str ());
	} else if (k == "length") {
		length = atof (v.c_str ());
	} else if (k == "audio_channels") {
		audio_channels = atoi (v.c_str ());
	} else if (k == "audio_sample_rate") {
		audio_sample_rate = atoi (v.c_str ());
	} else if (k == "audio_sample_format") {
		audio_sample_format = audio_sample_format_from_string (v);
	}
}


/** @param n A thumb index.
 *  @return The path to the thumb's image file.
 */
string
FilmState::thumb_file (int n) const
{
	return thumb_file_for_frame (thumb_frame (n));
}

/** @param n A frame index within the Film.
 *  @return The path to the thumb's image file for this frame;
 *  we assume that it exists.
 */
string
FilmState::thumb_file_for_frame (int n) const
{
	stringstream s;
	s << dir ("thumbs") << "/";
	s.width (8);
	s << setfill('0') << n << ".tiff";
	return s.str ();
}


/** @param n A thumb index.
 *  @return The frame within the Film that it is for.
 */
int
FilmState::thumb_frame (int n) const
{
	assert (n < int (thumbs.size ()));
	return thumbs[n];
}

Size
FilmState::cropped_size (Size s) const
{
	s.width -= left_crop + right_crop;
	s.height -= top_crop + bottom_crop;
	return s;
}

/** Given a directory name, return its full path within the Film's directory.
 *  The directory (and its parents) will be created if they do not exist.
 */
string
FilmState::dir (string d) const
{
	stringstream s;
	s << directory << "/" << d;
	filesystem::create_directories (s.str ());
	return s.str ();
}

/** Given a file or directory name, return its full path within the Film's directory */
string
FilmState::file (string f) const
{
	stringstream s;
	s << directory << "/" << f;
	return s.str ();
}

string
FilmState::content_path () const
{
	if (filesystem::path(content).has_root_directory ()) {
		return content;
	}

	return file (content);
}

ContentType
FilmState::content_type () const
{
#if BOOST_FILESYSTEM_VERSION == 3
	string const ext = filesystem::path(content).extension().string();
#else
	string const ext = filesystem::path(content).extension();
#endif
	if (ext == ".tif" || ext == ".tiff" || ext == ".jpg" || ext == ".jpeg" || ext == ".png") {
		return STILL;
	}

	return VIDEO;
}
