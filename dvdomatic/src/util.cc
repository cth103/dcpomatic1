/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>
    Copyright (C) 2000-2007 Paul Davis

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

#include <sstream>
#include <iomanip>
#include <iostream>
#include <execinfo.h>
#include <cxxabi.h>
#include <boost/algorithm/string.hpp>
#include <openjpeg.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libpostproc/postprocess.h>
}
#include "util.h"
#include "exceptions.h"

using namespace std;
using namespace boost;

int const YUVImage::components = 4;

/** Convert some number of seconds to a string representation
 *  in hours, minutes and seconds.
 *
 *  @param s Seconds.
 *  @return String of the form H:M:S (where H is hours, M
 *  is minutes and S is seconds).
 */
 
string
seconds_to_hms (int s)
{
	int m = s / 60;
	s -= (m * 60);
	int h = m / 60;
	m -= (h * 60);

	stringstream hms;
	hms << h << ":";
	hms.width (2);
	hms << setfill ('0') << m << ":";
	hms.width (2);
	hms << setfill ('0') << s;

	return hms.str ();
}

string
seconds_to_approximate_hms (int s)
{
	int m = s / 60;
	s -= (m * 60);
	int h = m / 60;
	m -= (h * 60);

	stringstream ap;
	
	if (h > 0) {
		if (m > 30) {
			ap << (h + 1) << " hours";
		} else {
			ap << h << " hours";
		}
	} else if (m > 0) {
		ap << m << " minutes";
	} else {
		ap << s << " seconds";
	}

	return ap.str ();
}

Gtk::Label &
left_aligned_label (string t)
{
	Gtk::Label* l = Gtk::manage (new Gtk::Label (t));
	l->set_alignment (0, 0.5);
	return *l;
}

static string
demangle (string l)
{
	string::size_type const b = l.find_first_of ("(");
	if (b == string::npos) {
		return l;
	}

	string::size_type const p = l.find_last_of ("+");
	if (p == string::npos) {
		return l;
	}

	if ((p - b) <= 1) {
		return l;
	}
	
	string const fn = l.substr (b + 1, p - b - 1);

	int status;
	try {
		
		char* realname = abi::__cxa_demangle (fn.c_str(), 0, 0, &status);
		string d (realname);
		free (realname);
		return d;
		
	} catch (std::exception) {
		
	}
	
	return l;
}

void
stacktrace (ostream& out, int levels)
{
	void *array[200];
	size_t size;
	char **strings;
	size_t i;
     
	size = backtrace (array, 200);
	strings = backtrace_symbols (array, size);
     
	if (strings) {
		for (i = 0; i < size && (levels == 0 || i < size_t(levels)); i++) {
			out << "  " << demangle (strings[i]) << endl;
		}
		
		free (strings);
	}
}

string
audio_sample_format_to_string (AVSampleFormat s)
{
	/* Our sample format handling is not exactly complete */
	
	switch (s) {
	case AV_SAMPLE_FMT_S16:
		return "S16";
	default:
		break;
	}

	return "Unknown";
}

AVSampleFormat
audio_sample_format_from_string (string s)
{
	if (s == "S16") {
		return AV_SAMPLE_FMT_S16;
	}

	return AV_SAMPLE_FMT_NONE;
}

static string
opendcp_version ()
{
	FILE* f = popen ("opendcp_xml", "r");
	if (f == 0) {
		throw EncodeError ("could not run opendcp_xml to check version");
	}

	string version = "unknown";
	
	while (!feof (f)) {
		char* buf = 0;
		size_t n = 0;
		getline (&buf, &n, f);
		if (n > 0) {
			string s (buf);
			vector<string> b;
			split (b, s, is_any_of (" "));
			if (b.size() >= 3 && b[0] == "OpenDCP" && b[1] == "version") {
				version = b[2];
			}
		}
	}

	pclose (f);

	return version;
}

static string
ffmpeg_version_to_string (int v)
{
	stringstream s;
	s << ((v & 0xff0000) >> 16) << "." << ((v & 0xff00) >> 8) << "." << (v & 0xff);
	return s.str ();
}

string
dependency_version_summary ()
{
	stringstream s;
	s << "libopenjpeg " << opj_version () << ", "
	  << "opendcp " << opendcp_version () << ", "
	  << "libswresample " << ffmpeg_version_to_string (swresample_version()) << ", "
	  << "libavcodec " << ffmpeg_version_to_string (avcodec_version()) << ", "
	  << "libavfilter " << ffmpeg_version_to_string (avfilter_version()) << ", "
	  << "libavformat " << ffmpeg_version_to_string (avformat_version()) << ", "
	  << "libavutil " << ffmpeg_version_to_string (avutil_version()) << ", "
	  << "libpostproc " << ffmpeg_version_to_string (postproc_version()) << ", "
	  << "libswscale " << ffmpeg_version_to_string (swscale_version());

	return s.str ();
}

void
fd_write (int fd, uint8_t const * data, int size)
{
	uint8_t const * p = data;
	while (size) {
		int const n = write (fd, p, size);
		if (n < 0) {
			stringstream s;
			s << "could not write (" << strerror (errno) << ")";
			throw NetworkError (s.str ());
		}

		size -= n;
		p += n;
	}
}

SocketReader::SocketReader (int fd)
	: _fd (fd)
	, _buffer_data (0)
{

}

void
SocketReader::consume (int size)
{
	_buffer_data -= size;
	if (_buffer_data > 0) {
		memmove (_buffer, _buffer + size, _buffer_data);
	}
}

void
SocketReader::read_definite_and_consume (uint8_t* data, int size)
{
	int const from_buffer = min (_buffer_data, size);
	if (from_buffer > 0) {
		/* Get data from our buffer */
		memcpy (data, _buffer, from_buffer);
		consume (from_buffer);
		/* Update our output state */
		data += from_buffer;
		size -= from_buffer;
	}

	/* read() the rest */
	while (size > 0) {
		int const n = ::read (_fd, data, size);
		if (n < 0) {
			throw NetworkError ("could not read");
		}
			
		data += n;
		size -= n;
	}
}

void
SocketReader::read_indefinite (uint8_t* data, int size)
{
	assert (size < int (sizeof (_buffer)));

	/* Amount of extra data we need to read () */
	int to_read = size - _buffer_data;
	while (to_read > 0) {
		/* read as much of it as we can (into our buffer) */
		int const n = ::read (_fd, _buffer + _buffer_data, to_read);
		if (n < 0) {
			throw NetworkError ("could not read");
		}

		to_read -= n;
		_buffer_data += n;
	}

	assert (_buffer_data >= size);

	/* copy data into the output buffer */
	assert (size >= _buffer_data);
	memcpy (data, _buffer, size);
}

/** Create an empty YUVImage with a given set of line sizes */
YUVImage::YUVImage (int const * ls, Size s, PixelFormat pix)
	: _size (s)
	, _pixel_format (pix)
{
	_line_size = new int[components];
	_data = new uint8_t*[components];
	
	for (int i = 0; i < components; ++i) {
		_line_size[i] = ls[i];
		if (_line_size[i] > 0) {
			_data[i] = (uint8_t *) av_malloc (_line_size[i] * _size.height);;
			printf("new[] at %p %d\n", _data[i], _line_size[i] * _size.height);
		} else {
			_data[i] = 0;
		}
	}

	_our_data = true;
}

/** Create a YUVImage by shallow copying existing data */
YUVImage::YUVImage (uint8_t** data, int const * ls, Size s, PixelFormat pix)
	: _size (s)
	, _pixel_format (pix)
{
	_line_size = new int[components];
	_data = new uint8_t*[components];
	
	for (int i = 0; i < components; ++i) {
		_line_size[i] = ls[i];
		_data[i] = data[i];
	}

	_our_data = false;
}

shared_ptr<YUVImage>
YUVImage::deep_copy ()
{
	shared_ptr<YUVImage> copy (new YUVImage (_line_size, _size, _pixel_format));
	for (int i = 0; i < components; ++i) {
		assert ((copy->data(i) && _data[i]) || (copy->data(i) == 0 && _data[i] == 0));
		if (copy->data(i) && _data[i]) {
			printf ("memcpy from %p %d to %p\n", _data[i], _line_size[i] * _size.height, copy->data (i));
			memcpy (copy->data (i), _data[i], _line_size[i] * _size.height);
			printf ("memcpy ok\n");
		}
		cout << "ok.\n";
	}

	return copy;
}

YUVImage::~YUVImage ()
{
	if (_our_data) {
		for (int i = 0; i < components; ++i) {
			av_free (_data[i]);
		}
	}

	printf ("delete[] %p\n", _data);
	delete[] _data;
	delete[] _line_size;
}
