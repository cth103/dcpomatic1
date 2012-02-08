#include <stdexcept>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <boost/filesystem.hpp>
#include "film.h"
#include "format.h"

using namespace std;

Film::Film (string const & d)
	: _directory (d)
	, _format (0)
	, _left_crop (0)
	, _right_crop (0)
	, _top_crop (0)
	, _bottom_crop (0)
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

	f << "content " << _content << "\n";
	if (_format) {
		f << "format " << _format->get_as_metadata () << "\n";
	}
	f << "top_crop " << _top_crop << "\n";
	f << "bottom_crop " << _bottom_crop << "\n";
	f << "left_crop " << _left_crop << "\n";
	f << "right_crop " << _right_crop << "\n";
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
Film::dir (string const & d)
{
	stringstream s;
	s << _directory << "/" << d;
	boost::filesystem::create_directories (s.str ());
	return s.str ();
}

