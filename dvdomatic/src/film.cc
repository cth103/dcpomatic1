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

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "film.h"
#include "format.h"
#include "tiff_transcoder.h"
#include "job.h"

using namespace std;

/** Construct a Film object in a given directory, reading any metadat
 *  file that exists in that directory.
 *
 *  @param d Film directory.
 */

Film::Film (string const & d)
	: _directory (d)
	, _dcp_content_type (0)
	, _format (0)
	, _left_crop (0)
	, _right_crop (0)
	, _top_crop (0)
	, _bottom_crop (0)
	, _width (0)
	, _height (0)
	, _frames_per_second (0)
{
	read_metadata ();
}

/** Read the `metadata' file inside this Film's directory, and fill the
 *  object's data with its content.
 */

void
Film::read_metadata ()
{
	ifstream f (metadata_file().c_str ());
	string line;
	while (getline (f, line)) {
		if (line.empty ()) {
			continue;
		}
		
		if (line[0] == '#') {
			continue;
		}

		size_t const s = line.find (' ');
		if (s == string::npos) {
			continue;
		}

		string const k = line.substr (0, s);
		string const v = line.substr (s + 1);

		/* User-specified stuff */
		if (k == "name") {
			_name = v;
		} else if (k == "content") {
			_content = v;
		} else if (k == "format") {
			_format = Format::get_from_metadata (v);
		} else if (k == "top_crop") {
			_top_crop = atoi (v.c_str ());
		} else if (k == "bottom_crop") {
			_bottom_crop = atoi (v.c_str ());
		} else if (k == "left_crop") {
			_left_crop = atoi (v.c_str ());
		} else if (k == "right_crop") {
			_right_crop = atoi (v.c_str ());
		} else if (k == "dcp_long_name") {
			_dcp_long_name = v;
		} else if (k == "dcp_pretty_name") {
			_dcp_pretty_name = v;
		} else if (k == "dcp_content_type") {
			_dcp_content_type = ContentType::get_from_pretty_name (v);
		}

		/* Cached stuff */
		if (k == "thumb") {
			int const n = atoi (v.c_str ());
			/* Only add it to the list if it still exists */
			if (boost::filesystem::exists (thumb_file_for_frame (n))) {
				_thumbs.push_back (n);
			}
		} else if (k == "width") {
			_width = atoi (v.c_str ());
		} else if (k == "height") {
			_height = atoi (v.c_str ());
		} else if (k == "frames_per_second") {
			_frames_per_second = atof (v.c_str ());
		}
	}
}

void
Film::write_metadata () const
{
	boost::filesystem::create_directories (_directory);
	
	ofstream f (metadata_file().c_str ());
	if (!f.good ()) {
		throw runtime_error ("Could not open metadata file for writing");
	}

	/* User stuff */
	f << "name " << _name << "\n";
	f << "content " << _content << "\n";
	if (_format) {
		f << "format " << _format->get_as_metadata () << "\n";
	}
	f << "top_crop " << _top_crop << "\n";
	f << "bottom_crop " << _bottom_crop << "\n";
	f << "left_crop " << _left_crop << "\n";
	f << "right_crop " << _right_crop << "\n";

	/* Cached stuff */
	for (vector<int>::const_iterator i = _thumbs.begin(); i != _thumbs.end(); ++i) {
		f << "thumb " << *i << "\n";
	}
	f << "width " << _width << "\n";
	f << "height " << _height << "\n";
	f << "frames_per_second " << _frames_per_second << "\n";
	f << "dcp_long_name " << _dcp_long_name << "\n";
	f << "dcp_pretty_name " << _dcp_pretty_name << "\n";
	if (_dcp_content_type) {
		f << "dcp_content_type " << _dcp_content_type->pretty_name () << "\n";
	}
}

string
Film::metadata_file () const
{
	return file ("metadata");
}

void
Film::set_top_crop (int c)
{
	if (c == _top_crop) {
		return;
	}
	
	_top_crop = c;
	Changed (TopCrop);
}

void
Film::set_bottom_crop (int c)
{
	if (c == _bottom_crop) {
		return;
	}
	
	_bottom_crop = c;
	Changed (BottomCrop);
}

void
Film::set_left_crop (int c)
{
	if (c == _left_crop) {
		return;
	}
	
	_left_crop = c;
	Changed (LeftCrop);
}

void
Film::set_right_crop (int c)
{
	if (c == _right_crop) {
		return;
	}
	
	_right_crop = c;
	Changed (RightCrop);
}

void
Film::set_format (Format* f)
{
	_format = f;
	Changed (FilmFormat);
}

void
Film::set_dcp_long_name (string const & n)
{
	_dcp_long_name = n;
	Changed (DCPLongName);
}

void
Film::set_dcp_pretty_name (string const & n)
{
	_dcp_pretty_name = n;
	Changed (DCPPrettyName);
}

void
Film::set_dcp_content_type (ContentType* t)
{
	_dcp_content_type = t;
	Changed (ContentTypeChange);
}

string
Film::content () const
{
	return file (_content);
}

void
Film::set_name (string const & n)
{
	_name = n;
}

/** Set the content file for this film.
 *  @param c New content file; if specified as an absolute path, the content should
 *  be within the film's _directory; if specified as a relative path, the content
 *  will be assumed to be within the film's _directory.
 */
void
Film::set_content (string const & c)
{
	bool const absolute = boost::filesystem::path(c).has_root_directory ();
	if (absolute && !boost::starts_with (c, _directory)) {
		throw runtime_error ("content is outside the film's directory");
	}

	string f = c;
	if (absolute) {
		f = f.substr (_directory.length());
	}
	
	if (f == _content) {
		return;
	}
	
	_content = f;
	Changed (Content);

	/* Create a temporary transcoder so that we can get information
	   about the content.
	*/
	Transcoder d (this, 0, 1024, 1024);
	_width = d.native_width ();
	_height = d.native_height ();
	_frames_per_second = d.frames_per_second ();

	Changed (Size);
	Changed (FramesPerSecond);
}

string
Film::dir (string const & d) const
{
	stringstream s;
	s << _directory << "/" << d;
	boost::filesystem::create_directories (s.str ());
	return s.str ();
}

string
Film::file (string const & f) const
{
	stringstream s;
	s << _directory << "/" << f;
	return s.str ();
}

void
Film::update_thumbs_non_gui (Job* job)
{
	if (_content.empty ()) {
		if (job) {
			job->set_progress (1);
		}
		return;
	}
	
	_thumbs.clear ();
	
	using namespace boost::filesystem;
	remove_all (dir ("thumbs"));

	int const number = 128;

	/* This call will recreate the directory */
	string const tdir = dir ("thumbs");

	job->descend (1);
	TIFFTranscoder w (this, job, _width, _height, tdir);
	w.apply_crop (false);
	w.set_decode_video_period (w.length_in_frames() / number);
	w.go ();
	job->ascend ();

	for (directory_iterator i = directory_iterator (tdir); i != directory_iterator(); ++i) {

		/* Aah, the sweet smell of progress */
#if BOOST_FILESYSTEM_VERSION == 3		
		string const l = path(*i).leaf().generic_string();
#else
		string const l = i->leaf ();
#endif
		
		size_t const d = l.find (".tiff");
		if (d != string::npos) {
			_thumbs.push_back (atoi (l.substr (0, d).c_str()));
		}
	}

	sort (_thumbs.begin(), _thumbs.end());
	write_metadata ();
}

void
Film::update_thumbs_gui ()
{
	ThumbsChanged ();
}

int
Film::num_thumbs () const
{
	return _thumbs.size ();
}

int
Film::thumb_frame (int n) const
{
	assert (n < int (_thumbs.size ()));
	return _thumbs[n];
}

string
Film::thumb_file (int n) const
{
	return thumb_file_for_frame (thumb_frame (n));
}

string
Film::thumb_file_for_frame (int n) const
{
	stringstream s;
	s << dir("thumbs") << "/";
	s.width (8);
	s << setfill('0') << n << ".tiff";
	return s.str ();
}

string
Film::j2k_sub_directory () const
{
	assert (_format);
	
	stringstream s;
	s << _format->nickname() << "_" << _content << "_" << _left_crop << "_" << _right_crop << "_" << _top_crop << "_" << _bottom_crop;
	return s.str ();
}


string
Film::j2k_dir () const
{
	stringstream s;
	s << "j2c/" << j2k_sub_directory();
	return dir (s.str ());
}

string
Film::j2k_path (int f, bool tmp) const
{
	stringstream s;
	s << j2k_dir() << "/";
	s.width (8);
	s << setfill('0') << f << ".j2c";

	if (tmp) {
		s << ".tmp";
	}

	return s.str ();
}
