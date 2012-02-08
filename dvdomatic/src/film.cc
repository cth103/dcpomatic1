#include <stdexcept>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <boost/filesystem.hpp>
#include "film.h"
#include "format.h"
#include "progress.h"
#include "film_writer.h"

using namespace std;

Film::Film (string const & d)
	: _directory (d)
	, _format (0)
	, _left_crop (0)
	, _right_crop (0)
	, _top_crop (0)
	, _bottom_crop (0)
	, _thumb_width (0)
	, _thumb_height (0)
{
	read_metadata ();
}

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
		if (k == "content") {
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
		}

		/* Cached stuff */
		if (k == "thumb") {
			_thumbs.push_back (atoi (v.c_str ()));
		} else if (k == "thumb_width") {
			_thumb_width = atoi (v.c_str ());
		} else if (k == "thumb_height") {
			_thumb_height = atoi (v.c_str ());
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
	f << "thumb_width " << _thumb_width << "\n";
	f << "thumb_height " << _thumb_height << "\n";
}

string
Film::metadata_file () const
{
	stringstream s;
	s << _directory << "/metadata";
	return s.str();
}

void
Film::set_top_crop (int c)
{
	_top_crop = c;
}

void
Film::set_bottom_crop (int c)
{
	_bottom_crop = c;
}

void
Film::set_left_crop (int c)
{
	_left_crop = c;
}

void
Film::set_right_crop (int c)
{
	_right_crop = c;
}

void
Film::set_format (Format* f)
{
	_format = f;
}

string
Film::content () const
{
	stringstream s;
	s << _directory << "/" << _content;
	return s.str ();
}

void
Film::set_content (string const & c)
{
	_content = c;
}

string
Film::dir (string const & d) const
{
	stringstream s;
	s << _directory << "/" << d;
	boost::filesystem::create_directories (s.str ());
	return s.str ();
}

void
Film::update_thumbs ()
{
	_thumbs.clear ();
	
	using namespace boost::filesystem;
	remove_all (dir ("thumbs"));

	_thumb_height = 256;
	int const number = 128;
	
	_thumb_width = _thumb_height * _format->ratio_as_float ();

	/* This call will recreate the directory */
	string const tdir = dir ("thumbs");
	
	Progress progress;
	FilmWriter w (this, &progress, _thumb_width, _thumb_height, tdir, tdir);
	w.decode_audio (false);
	w.set_decode_video_period (w.length_in_frames() / number);
	w.go ();

	for (directory_iterator i = directory_iterator (tdir); i != directory_iterator(); ++i) {
		string const l = i->leaf ();
		size_t const d = l.find (".tiff");
		if (d != string::npos) {
			_thumbs.push_back (atoi (l.substr (0, d).c_str()));
		}
	}

	sort (_thumbs.begin(), _thumbs.end());

	write_metadata ();
}

int
Film::num_thumbs () const
{
	return _thumbs.size ();
}

int
Film::thumb_frame (int n) const
{
	assert (n <= int (_thumbs.size ()));
	return _thumbs[n];
}

string
Film::thumb_file (int n) const
{
	stringstream s;
	s << dir("thumbs") << "/";
	s.width (8);
	s << setfill('0') << thumb_frame (n) << ".tiff";
	return s.str ();
}
