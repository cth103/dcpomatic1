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
	, _width (0)
	, _height (0)
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
		}

		/* Cached stuff */
		if (k == "thumb") {
			_thumbs.push_back (atoi (v.c_str ()));
		} else if (k == "width") {
			_width = atoi (v.c_str ());
		} else if (k == "height") {
			_height = atoi (v.c_str ());
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
	if (c == _top_crop) {
		return;
	}
	
	_top_crop = c;
	CropChanged ();
}

void
Film::set_bottom_crop (int c)
{
	if (c == _bottom_crop) {
		return;
	}
	
	_bottom_crop = c;
	CropChanged ();
}

void
Film::set_left_crop (int c)
{
	if (c == _left_crop) {
		return;
	}
	
	_left_crop = c;
	CropChanged ();
}

void
Film::set_right_crop (int c)
{
	if (c == _right_crop) {
		return;
	}
	
	_right_crop = c;
	CropChanged ();
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
Film::set_name (string const & n)
{
	_name = n;
}

void
Film::set_content (string const & c)
{
	bool const absolute = boost::filesystem::path(c).has_root_directory ();
	if (absolute && !boost::starts_with (c, _directory)) {
		throw runtime_error ("Content is outside the film's directory");
	}

	string f = c;
	if (absolute) {
		f = f.substr (_directory.length());
	}
	
	if (f == _content) {
		return;
	}
	
	_content = f;

	Decoder d (this, 1, 1);
	_width = d.native_width ();
	_height = d.native_height ();
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
Film::update_thumbs (Progress* progress)
{
	_thumbs.clear ();
	
	using namespace boost::filesystem;
	remove_all (dir ("thumbs"));

	int const number = 128;

	/* This call will recreate the directory */
	string const tdir = dir ("thumbs");
	
	FilmWriter w (this, progress, _width, _height, tdir, tdir);
	w.decode_audio (false);
	w.apply_crop (false);
	w.set_decode_video_period (w.length_in_frames() / number);
	w.go ();

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

	ThumbsChanged ();
	progress->set_done (true);
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
	stringstream s;
	s << dir("thumbs") << "/";
	s.width (8);
	s << setfill('0') << thumb_frame (n) << ".tiff";
	return s.str ();
}
