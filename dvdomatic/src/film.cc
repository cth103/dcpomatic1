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
#include <unistd.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <openjpeg.h>
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
#include "film_state.h"
#include "log.h"
#include "options.h"
#include "exceptions.h"
#include "version.h"

using namespace std;
using namespace boost;

/** Construct a Film object in a given directory, reading any metadata
 *  file that exists in that directory.  An exception will be thrown if
 *  must_exist is true, and the specified directory does not exist.
 *
 *  @param d Film directory.
 *  @param must_exist true to throw an exception if does not exist.
 */

Film::Film (string d, bool must_exist)
	: _dirty (false)
{
	/* Make _state.directory a complete path without ..s (where possible)
	   (Code swiped from Adam Bowen on stackoverflow)
	*/
	
	filesystem::path p (filesystem::system_complete (d));
	filesystem::path result;
	for(filesystem::path::iterator i = p.begin(); i != p.end(); ++i) {
		if (*i == "..") {
			if (filesystem::is_symlink (result) || result.filename() == "..") {
				result /= *i;
			} else {
				result = result.parent_path ();
			}
		} else if (*i != ".") {
			result /= *i;
		}
	}

	_state.directory = result.string ();
	
	if (must_exist && !filesystem::exists (_state.directory)) {
		throw OpenFileError (_state.directory);
	}

	read_metadata ();

	_log = new Log (_state.file ("log"));
}

/** Copy constructor */
Film::Film (Film const & other)
	: _state (other._state)
	, _dirty (other._dirty)
{

}

Film::~Film ()
{
	delete _log;
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
			_state.name = v;
		} else if (k == "content") {
			_state.content = v;
		} else if (k == "dcp_long_name") {
			_state.dcp_long_name = v;
		} else if (k == "guess_dcp_long_name") {
			_state.guess_dcp_long_name = (v == "1");
		} else if (k == "dcp_content_type") {
			_state.dcp_content_type = ContentType::get_from_pretty_name (v);
		} else if (k == "format") {
			_state.format = Format::get_from_metadata (v);
		} else if (k == "left_crop") {
			_state.left_crop = atoi (v.c_str ());
		} else if (k == "right_crop") {
			_state.right_crop = atoi (v.c_str ());
		} else if (k == "top_crop") {
			_state.top_crop = atoi (v.c_str ());
		} else if (k == "bottom_crop") {
			_state.bottom_crop = atoi (v.c_str ());
		} else if (k == "filter") {
			_state.filters.push_back (Filter::get_from_id (v));
		} else if (k == "dcp_frames") {
			_state.dcp_frames = atoi (v.c_str ());
		} else if (k == "dcp_ab") {
			_state.dcp_ab = (v == "1");
		}

		/* Cached stuff */
		if (k == "thumb") {
			int const n = atoi (v.c_str ());
			/* Only add it to the list if it still exists */
			if (filesystem::exists (thumb_file_for_frame (n))) {
				_state.thumbs.push_back (n);
			}
		} else if (k == "width") {
			_state.width = atoi (v.c_str ());
		} else if (k == "height") {
			_state.height = atoi (v.c_str ());
		} else if (k == "length") {
			_state.length = atof (v.c_str ());
		} else if (k == "frames_per_second") {
			_state.frames_per_second = atof (v.c_str ());
		} else if (k == "audio_channels") {
			_state.audio_channels = atoi (v.c_str ());
		} else if (k == "audio_sample_rate") {
			_state.audio_sample_rate = atoi (v.c_str ());
		} else if (k == "audio_sample_format") {
			_state.audio_sample_format = audio_sample_format_from_string (v);
		}
	}

	_dirty = false;
}

/** Write our state to a file `metadata' inside the Film's directory */
void
Film::write_metadata () const
{
	filesystem::create_directories (_state.directory);
	
	ofstream f (metadata_file().c_str ());
	if (!f.good ()) {
		throw CreateFileError (metadata_file ());
	}

	/* User stuff */
	f << "name " << _state.name << "\n";
	f << "content " << _state.content << "\n";
	f << "dcp_long_name " << _state.dcp_long_name << "\n";
	f << "guess_dcp_long_name " << (_state.guess_dcp_long_name ? "1" : "0") << "\n";
	if (_state.dcp_content_type) {
		f << "dcp_content_type " << _state.dcp_content_type->pretty_name () << "\n";
	}
	if (_state.format) {
		f << "format " << _state.format->get_as_metadata () << "\n";
	}
	f << "left_crop " << _state.left_crop << "\n";
	f << "right_crop " << _state.right_crop << "\n";
	f << "top_crop " << _state.top_crop << "\n";
	f << "bottom_crop " << _state.bottom_crop << "\n";
	for (vector<Filter const *>::const_iterator i = _state.filters.begin(); i != _state.filters.end(); ++i) {
		f << "filter " << (*i)->id () << "\n";
	}
	f << "dcp_frames " << _state.dcp_frames << "\n";
	f << "dcp_ab " << (_state.dcp_ab ? "1" : "0") << "\n";

	/* Cached stuff; this is information about our content; we could
	   look it up each time, but that's slow.
	*/
	for (vector<int>::const_iterator i = _state.thumbs.begin(); i != _state.thumbs.end(); ++i) {
		f << "thumb " << *i << "\n";
	}
	f << "width " << _state.width << "\n";
	f << "height " << _state.height << "\n";
	f << "length " << _state.length << "\n";
	f << "frames_per_second " << _state.frames_per_second << "\n";
	f << "audio_channels " << _state.audio_channels << "\n";
	f << "audio_sample_rate " << _state.audio_sample_rate << "\n";
	f << "audio_sample_format " << audio_sample_format_to_string (_state.audio_sample_format) << "\n";

	_dirty = false;
}

/** Set the name by which DVD-o-matic refers to this Film */
void
Film::set_name (string n)
{
	_state.name = n;
	signal_changed (Name);
}

/** Set the content file for this film.
 *  @param c New content file; if specified as an absolute path, the content should
 *  be within the film's _state.directory; if specified as a relative path, the content
 *  will be assumed to be within the film's _state.directory.
 */
void
Film::set_content (string c)
{
	bool const absolute = filesystem::path(c).has_root_directory ();
	if (absolute && !starts_with (c, _state.directory)) {
		throw BadContentLocationError ();
	}

	string f = c;
	if (absolute) {
		f = f.substr (_state.directory.length());
	}

	if (f == _state.content) {
		return;
	}
	
	/* Create a temporary decoder so that we can get information
	   about the content.
	*/
	shared_ptr<FilmState> s = state_copy ();
	s->content = f;
	shared_ptr<Options> o (new Options ("", "", ""));
	o->out_width = 1024;
	o->out_height = 1024;

	Decoder d (s, o, 0);
	
	_state.width = d.native_width ();
	_state.height = d.native_height ();
	_state.length = d.length_in_frames ();
	_state.frames_per_second = d.frames_per_second ();
	_state.audio_channels = d.audio_channels ();
	_state.audio_sample_rate = d.audio_sample_rate ();
	_state.audio_sample_format = d.audio_sample_format ();

	_state.content = f;
	
	signal_changed (Size);
	signal_changed (Length);
	signal_changed (FramesPerSecond);
	signal_changed (AudioChannels);
	signal_changed (AudioSampleRate);
	signal_changed (Content);
}

/** Set the format that this Film should be shown in */
void
Film::set_format (Format* f)
{
	_state.format = f;
	signal_changed (FilmFormat);
}

/** Set the `long name' to use when generating the DCP
 *  (the one like THE-BLUES-BROS_FTR_F_EN-XX ...)
 */
void
Film::set_dcp_long_name (string n)
{
	_state.dcp_long_name = n;
	signal_changed (DCPLongName);
}

/** Set the type to specify the DCP as having
 *  (feature, trailer etc.)
 */
void
Film::set_dcp_content_type (ContentType const * t)
{
	_state.dcp_content_type = t;
	signal_changed (DCPContentType);
}

/** Set the number of pixels by which to crop the left of the source video */
void
Film::set_left_crop (int c)
{
	if (c == _state.left_crop) {
		return;
	}
	
	_state.left_crop = c;
	signal_changed (LeftCrop);
}

/** Set the number of pixels by which to crop the right of the source video */
void
Film::set_right_crop (int c)
{
	if (c == _state.right_crop) {
		return;
	}

	_state.right_crop = c;
	signal_changed (RightCrop);
}

/** Set the number of pixels by which to crop the top of the source video */
void
Film::set_top_crop (int c)
{
	if (c == _state.top_crop) {
		return;
	}
	
	_state.top_crop = c;
	signal_changed (TopCrop);
}

/** Set the number of pixels by which to crop the bottom of the source video */
void
Film::set_bottom_crop (int c)
{
	if (c == _state.bottom_crop) {
		return;
	}
	
	_state.bottom_crop = c;
	signal_changed (BottomCrop);
}

/** Set the filters to apply to the image when generating thumbnails
 *  or a DCP.
 */
void
Film::set_filters (vector<Filter const *> const & f)
{
	_state.filters = f;
	signal_changed (Filters);
}

/** Set the number of frames to put in any generated DCP (from
 *  the start of the film).  0 indicates that all frames should
 *  be used.
 */
void
Film::set_dcp_frames (int n)
{
	_state.dcp_frames = n;
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
	_state.dcp_ab = a;
	signal_changed (DCPAB);
}

/** @return path of metadata file */
string
Film::metadata_file () const
{
	return _state.file ("metadata");
}

/** @return full path of the content (actual video) file
 *  of this Film.
 */
string
Film::content () const
{
	return _state.file (_state.content);
}

/** The pre-processing GUI part of a thumbs update.
 *  Must be called from the GUI thread.
 */
void
Film::update_thumbs_pre_gui ()
{
	_state.thumbs.clear ();
	filesystem::remove_all (_state.dir ("thumbs"));

	/* This call will recreate the directory */
	_state.dir ("thumbs");
}

/** The post-processing GUI part of a thumbs update.
 *  Must be called from the GUI thread.
 */
void
Film::update_thumbs_post_gui ()
{
	string const tdir = _state.dir ("thumbs");
	
	for (filesystem::directory_iterator i = filesystem::directory_iterator (tdir); i != filesystem::directory_iterator(); ++i) {

		/* Aah, the sweet smell of progress */
#if BOOST_FILESYSTEM_VERSION == 3		
		string const l = filesystem::path(*i).leaf().generic_string();
#else
		string const l = i->leaf ();
#endif
		
		size_t const d = l.find (".tiff");
		if (d != string::npos) {
			_state.thumbs.push_back (atoi (l.substr (0, d).c_str()));
		}
	}

	sort (_state.thumbs.begin(), _state.thumbs.end());
	
	write_metadata ();
	signal_changed (Thumbs);
}

/** @return the number of thumbnail images that we have */
int
Film::num_thumbs () const
{
	return _state.thumbs.size ();
}

/** @param n A thumb index.
 *  @return The frame within the Film that it is for.
 */
int
Film::thumb_frame (int n) const
{
	assert (n < int (num_thumbs ()));
	return _state.thumbs[n];
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
	s << _state.dir ("thumbs") << "/";
	s.width (8);
	s << setfill('0') << n << ".tiff";
	return s.str ();
}

/** @return The path to the directory to write JPEG2000 files to */
string
Film::j2k_dir () const
{
	assert (format());

	stringstream s;

	/* Start with j2c */
	s << "j2c/";

	pair<string, string> f = Filter::ffmpeg_strings (filters ());

	/* Write stuff to specify the filter / post-processing settings that are in use,
	   so that we don't get confused about J2K files generated using different
	   settings.
	*/
	s << _state.format->nickname()
	  << "_" << _state.content
	  << "_" << left_crop() << "_" << right_crop() << "_" << top_crop() << "_" << bottom_crop()
	  << "_" << f.first << "_" << f.second;

	/* Similarly for the A/B case */
	if (dcp_ab()) {
		s << "/ab";
	}
	
	return _state.dir (s.str ());
}

/** Handle a change to the Film's metadata */
void
Film::signal_changed (Property p)
{
	_dirty = true;

	if (p == Name || p == DCPContentType || p == FilmFormat || p == AudioChannels) {
		maybe_guess_dcp_long_name ();
	}
	
	Changed (p);
}

/** Add suitable Jobs to the JobManager to create a DCP for this Film */
void
Film::make_dcp ()
{
	FILE* f = popen ("opendcp_xml", "r");
	if (f == 0) {
		throw EncodeError ("could not run opendcp_xml to check version");
	}

	string opendcp_version = "unknown";
	
	while (!feof (f)) {
		char* buf = 0;
		size_t n = 0;
		getline (&buf, &n, f);
		if (n > 0) {
			string s (buf);
			vector<string> b;
			split (b, s, is_any_of (" "));
			if (b.size() >= 3 && b[0] == "OpenDCP" && b[1] == "version") {
				opendcp_version = b[2];
			}
		}
	}

	pclose (f);

	{
		stringstream s;
		s << "DVD-o-matic version " << DVDOMATIC_VERSION
		  << " using libopenjpeg " << opj_version ()
		  << ", opendcp " << opendcp_version;
		log()->log (s.str ());
	}

	{
		char buffer[128];
		gethostname (buffer, sizeof (buffer));
		stringstream s;
		s << "Starting to make a DCP on " << buffer;
		log()->log (s.str ());
	}
		
	if (format() == 0) {
		throw MissingSettingError ("format");
	}

	if (content().empty ()) {
		throw MissingSettingError ("content");
	}

	if (dcp_content_type() == 0) {
		throw MissingSettingError ("content type");
	}

	shared_ptr<const FilmState> fs = state_copy ();
	shared_ptr<Options> o (new Options (j2k_dir(), ".j2c", _state.dir ("wavs")));
	o->out_width = format()->dci_width ();
	o->out_height = format()->dci_height ();
	o->num_frames = dcp_frames ();
	
	if (_state.dcp_ab) {
		JobManager::instance()->add (new ABTranscodeJob (fs, o, log ()));
	} else {
		JobManager::instance()->add (new TranscodeJob (fs, o, log ()));
	}
	
	JobManager::instance()->add (new MakeMXFJob (fs, o, log (), MakeMXFJob::VIDEO));
	JobManager::instance()->add (new MakeMXFJob (fs, o, log (), MakeMXFJob::AUDIO));
	JobManager::instance()->add (new MakeDCPJob (fs, o, log ()));
}

shared_ptr<FilmState>
Film::state_copy () const
{
	return shared_ptr<FilmState> (new FilmState (_state));
}

void
Film::set_guess_dcp_long_name (bool g)
{
	_state.guess_dcp_long_name = g;
	maybe_guess_dcp_long_name ();
	signal_changed (GuessDCPLongName);
}

void
Film::maybe_guess_dcp_long_name ()
{
	if (!_state.guess_dcp_long_name) {
		return;
	}

	stringstream s;

	string short_name = _state.name.substr (0, 14);
	to_upper (short_name);
	replace_all (short_name, " ", "-");
	s << short_name;

	if (_state.dcp_content_type) {
		s << "_" << _state.dcp_content_type->dcp_name ();
	}

	if (_state.format) {
		s << "_" << _state.format->dcp_name ();
	}

	s << "_EN-XX_GB";

	switch (_state.audio_channels) {
	case 1:
		s << "_10-EN";
		break;
	case 2:
		s << "_20-EN";
		break;
	case 5:
		s << "_51-EN";
		break;
	}

	s << "_2K_ST";

	time_t t;
	time (&t);
	struct tm * ts = localtime (&t);
	s << "_" << (ts->tm_year + 1900);
	s.width (2);
	s << setfill('0') << (ts->tm_mon + 1) << ts->tm_mday;

	s << "_FAC_2D_OV";
	
	set_dcp_long_name (s.str ());
}
