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
#include "tiff_encoder.h"
#include "job.h"
#include "filter.h"
#include "transcoder.h"
#include "util.h"
#include "job_manager.h"
#include "ab_transcode_job.h"
#include "transcode_job.h"
#include "make_mxf_job.h"
#include "make_dcp_job.h"
#include "parameters.h"

using namespace std;

/** Construct a Film object in a given directory, reading any metadata
 *  file that exists in that directory.  An exception will be thrown if
 *  must_exist is true, and the specified directory does not exist.
 *
 *  @param d Film directory.
 *  @param must_exist true to throw an exception if does not exist.
 */

Film::Film (string const & d, bool must_exist)
	: _dcp_content_type (0)
	, _format (0)
	, _left_crop (0)
	, _right_crop (0)
	, _top_crop (0)
	, _bottom_crop (0)
	, _dcp_frames (0)
	, _dcp_ab (false)
	, _width (0)
	, _height (0)
	, _frames_per_second (0)
	, _audio_channels (0)
	, _audio_sample_rate (0)
	, _audio_sample_format (AV_SAMPLE_FMT_NONE)
	, _dirty (false)
{
	/* Make _directory a complete path, as this is assumed elsewhere */
	_directory = boost::filesystem::system_complete (d).string ();
	
	if (must_exist && !boost::filesystem::exists (_directory)) {
		throw runtime_error ("film not found");
	}

	read_metadata ();
}

/** Copy constructor */
Film::Film (Film const & other)
	: _directory (other._directory)
	, _name (other._name)
	, _content (other._content)
	, _dcp_long_name (other._dcp_long_name)
	, _dcp_pretty_name (other._dcp_pretty_name)
	, _dcp_content_type (other._dcp_content_type)
	, _format (other._format)
	, _left_crop (other._left_crop)
	, _right_crop (other._right_crop)
	, _top_crop (other._top_crop)
	, _bottom_crop (other._bottom_crop)
	, _filters (other._filters)
	, _dcp_frames (other._dcp_frames)
	, _dcp_ab (other._dcp_ab)
	, _thumbs (other._thumbs)
	, _width (other._width)
	, _height (other._height)
	, _frames_per_second (other._frames_per_second)
	, _audio_channels (other._audio_channels)
	, _audio_sample_rate (other._audio_sample_rate)
	, _audio_sample_format (other._audio_sample_format)
	, _dirty (other._dirty)
{

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
		} else if (k == "dcp_long_name") {
			_dcp_long_name = v;
		} else if (k == "dcp_pretty_name") {
			_dcp_pretty_name = v;
		} else if (k == "dcp_content_type") {
			_dcp_content_type = ContentType::get_from_pretty_name (v);
		} else if (k == "format") {
			_format = Format::get_from_metadata (v);
		} else if (k == "left_crop") {
			_left_crop = atoi (v.c_str ());
		} else if (k == "right_crop") {
			_right_crop = atoi (v.c_str ());
		} else if (k == "top_crop") {
			_top_crop = atoi (v.c_str ());
		} else if (k == "bottom_crop") {
			_bottom_crop = atoi (v.c_str ());
		} else if (k == "filter") {
			_filters.push_back (Filter::get_from_id (v));
		} else if (k == "dcp_frames") {
			_dcp_frames = atoi (v.c_str ());
		} else if (k == "dcp_ab") {
			_dcp_ab = (v == "1");
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
		} else if (k == "audio_channels") {
			_audio_channels = atoi (v.c_str ());
		} else if (k == "audio_sample_rate") {
			_audio_sample_rate = atoi (v.c_str ());
		} else if (k == "audio_sample_format") {
			_audio_sample_format = audio_sample_format_from_string (v);
		}
	}

	_dirty = false;
}

/** Write our state to a file `metadata' inside the Film's directory */
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
	f << "dcp_long_name " << _dcp_long_name << "\n";
	f << "dcp_pretty_name " << _dcp_pretty_name << "\n";
	if (_dcp_content_type) {
		f << "dcp_content_type " << _dcp_content_type->pretty_name () << "\n";
	}
	if (_format) {
		f << "format " << _format->get_as_metadata () << "\n";
	}
	f << "left_crop " << _left_crop << "\n";
	f << "right_crop " << _right_crop << "\n";
	f << "top_crop " << _top_crop << "\n";
	f << "bottom_crop " << _bottom_crop << "\n";
	for (vector<Filter const *>::const_iterator i = _filters.begin(); i != _filters.end(); ++i) {
		f << "filter " << (*i)->id () << "\n";
	}
	f << "dcp_frames " << _dcp_frames << "\n";
	f << "dcp_ab " << (_dcp_ab ? "1" : "0") << "\n";

	/* Cached stuff; this is information about our content; we could
	   look it up each time, but that's slow.
	*/
	for (vector<int>::const_iterator i = _thumbs.begin(); i != _thumbs.end(); ++i) {
		f << "thumb " << *i << "\n";
	}
	f << "width " << _width << "\n";
	f << "height " << _height << "\n";
	f << "frames_per_second " << _frames_per_second << "\n";
	f << "audio_channels " << _audio_channels << "\n";
	f << "audio_sample_rate " << _audio_sample_rate << "\n";
	f << "audio_sample_format " << audio_sample_format_to_string (_audio_sample_format) << "\n";

	_dirty = false;
}

/** Set the name by which DVD-o-matic refers to this Film */
void
Film::set_name (string const & n)
{
	_name = n;
	signal_changed (Name);
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

	/* Create a temporary decoder so that we can get information
	   about the content.
	*/
	Parameters p ("", "", "");
	p.out_width = 1024;
	p.out_height = 1024;
	
	Decoder d (0, &p);

	_width = d.native_width ();
	_height = d.native_height ();
	_frames_per_second = d.frames_per_second ();
	_audio_channels = d.audio_channels ();
	_audio_sample_rate = d.audio_sample_rate ();
	_audio_sample_format = d.audio_sample_format ();
	
	signal_changed (Size);
	signal_changed (FramesPerSecond);
	signal_changed (AudioChannels);
	signal_changed (AudioSampleRate);
}

/** Set the format that this Film should be shown in */
void
Film::set_format (Format* f)
{
	_format = f;
	signal_changed (FilmFormat);
}

/** Set the `long name' to use when generating the DCP
 *  (the one like THE-BLUES-BROS_FTR_F_EN-XX ...)
 */
void
Film::set_dcp_long_name (string const & n)
{
	_dcp_long_name = n;
	signal_changed (DCPLongName);
}

/** Set the `pretty name' to use when generating the DCP.
 *  This will be displayed on most content servers.
 */
void
Film::set_dcp_pretty_name (string const & n)
{
	_dcp_pretty_name = n;
	signal_changed (DCPPrettyName);
}

/** Set the type to specify the DCP as having
 *  (feature, trailer etc.)
 */
void
Film::set_dcp_content_type (ContentType const * t)
{
	_dcp_content_type = t;
	signal_changed (DCPContentType);
}

/** Set the number of pixels by which to crop the left of the source video */
void
Film::set_left_crop (int c)
{
	if (c == _left_crop) {
		return;
	}
	
	_left_crop = c;
	signal_changed (LeftCrop);
}

/** Set the number of pixels by which to crop the right of the source video */
void
Film::set_right_crop (int c)
{
	if (c == _right_crop) {
		return;
	}

	_right_crop = c;
	signal_changed (RightCrop);
}

/** Set the number of pixels by which to crop the top of the source video */
void
Film::set_top_crop (int c)
{
	if (c == _top_crop) {
		return;
	}
	
	_top_crop = c;
	signal_changed (TopCrop);
}

/** Set the number of pixels by which to crop the bottom of the source video */
void
Film::set_bottom_crop (int c)
{
	if (c == _bottom_crop) {
		return;
	}
	
	_bottom_crop = c;
	signal_changed (BottomCrop);
}

/** Set the filters to apply to the image when generating thumbnails
 *  or a DCP.
 */
void
Film::set_filters (vector<Filter const *> const & f)
{
	_filters = f;
	signal_changed (Filters);
}

/** Set the number of frames to put in any generated DCP (from
 *  the start of the film).  0 indicates that all frames should
 *  be used.
 */
void
Film::set_dcp_frames (int n)
{
	_dcp_frames = n;
	signal_changed (DCPFrames);
}

/** Set whether or not to generate a A/B comparison DCP.
 *  Such a DCP has the left half of its frame as the Film
 *  content without any filtering or post-processing; the
 *  right half is rendered with filters and post-processing.
 */
void
Film::set_dcp_ab (bool a)
{
	_dcp_ab = a;
	signal_changed (DCPAB);
}

/** Given a file name, return its full path within the Film's directory */
string
Film::file (string const & f) const
{
	stringstream s;
	s << _directory << "/" << f;
	return s.str ();
}

/** Given a directory name, return its full path within the Film's directory.
 *  The directory (and its parents) will be created if they do not exist.
 */
string
Film::dir (string const & d) const
{
	stringstream s;
	s << _directory << "/" << d;
	boost::filesystem::create_directories (s.str ());
	return s.str ();
}

/** @return path of metadata file */
string
Film::metadata_file () const
{
	return file ("metadata");
}

/** @return full path of the content (actual video) file
 *  of this Film.
 */
string
Film::content () const
{
	return file (_content);
}

/** The pre-processing GUI part of a thumbs update.
 *  Must be called from the GUI thread.
 */
void
Film::update_thumbs_pre_gui ()
{
	_thumbs.clear ();
	boost::filesystem::remove_all (dir ("thumbs"));

	/* This call will recreate the directory */
	dir ("thumbs");
}

/** The post-processing GUI part of a thumbs update.
 *  Must be called from the GUI thread.
 */
void
Film::update_thumbs_post_gui ()
{
	string const tdir = dir ("thumbs");
	
	for (boost::filesystem::directory_iterator i = boost::filesystem::directory_iterator (tdir); i != boost::filesystem::directory_iterator(); ++i) {

		/* Aah, the sweet smell of progress */
#if BOOST_FILESYSTEM_VERSION == 3		
		string const l = boost::filesystem::path(*i).leaf().generic_string();
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
	signal_changed (Thumbs);
}

/** @return the number of thumbnail images that we have */
int
Film::num_thumbs () const
{
	return _thumbs.size ();
}

/** @param n A thumb index.
 *  @return The frame within the Film that it is for.
 */
int
Film::thumb_frame (int n) const
{
	assert (n < int (_thumbs.size ()));
	return _thumbs[n];
}

/** @param n A thumb index.
 *  @return The path to the thumb's image file.
 */
string
Film::thumb_file (int n) const
{
	return thumb_file_for_frame (thumb_frame (n));
}

/** @param n A frame index within the Film.
 *  @return The path to the thumb's image file for this frame;
 *  we assume that it exists.
 */
string
Film::thumb_file_for_frame (int n) const
{
	stringstream s;
	s << dir("thumbs") << "/";
	s.width (8);
	s << setfill('0') << n << ".tiff";
	return s.str ();
}

/** @return The path to the directory to write JPEG2000 files to */
string
Film::j2k_dir () const
{
	assert (_format);

	stringstream s;

	/* Start with j2c */
	s << "j2c/";

	pair<string, string> f = Filter::ffmpeg_strings (get_filters ());

	/* Write stuff to specify the filter / post-processing settings that are in use,
	   so that we don't get confused about J2K files generated using different
	   settings.
	*/
	s << _format->nickname()
	  << "_" << _content
	  << "_" << _left_crop << "_" << _right_crop << "_" << _top_crop << "_" << _bottom_crop
	  << "_" << f.first << "_" << f.second;

	/* Similarly for the A/B case */
	if (_dcp_ab) {
		s << "/ab";
	}
	
	return dir (s.str ());
}

/** Handle a change to the Film's metadata */
void
Film::signal_changed (Property p)
{
	_dirty = true;
	Changed (p);
}

/** Add suitable Jobs to the JobManager to create a DCP for this Film */
void
Film::make_dcp ()
{
	if (_format == 0) {
		throw runtime_error ("format must be specified to make a DCP");
	}
	
	if (_dcp_ab) {
		JobManager::instance()->add (new ABTranscodeJob (this, dcp_frames ()));
	} else {
		JobManager::instance()->add (new TranscodeJob (this, dcp_frames ()));
	}
	
	JobManager::instance()->add (new MakeMXFJob (this, MakeMXFJob::VIDEO));
	JobManager::instance()->add (new MakeMXFJob (this, MakeMXFJob::AUDIO));
	JobManager::instance()->add (new MakeDCPJob (this));
}
