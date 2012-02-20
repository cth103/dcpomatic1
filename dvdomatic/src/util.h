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

#ifndef DVDOMATIC_UTIL_H
#define DVDOMATIC_UTIL_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <gtkmm.h>
extern "C" {
#include <libavcodec/avcodec.h>
}

extern std::string seconds_to_hms (int);
extern std::string seconds_to_approximate_hms (int);
extern Gtk::Label & left_aligned_label (std::string);
extern void stacktrace (std::ostream &, int);
extern std::string audio_sample_format_to_string (AVSampleFormat);
extern AVSampleFormat audio_sample_format_from_string (std::string);
extern std::string dependency_version_summary ();
extern void fd_write (int, uint8_t const *, int);

class SocketReader
{
public:
	SocketReader (int);

	void read_definite_and_consume (uint8_t *, int);
	void read_indefinite (uint8_t *, int);
	void consume (int);

private:
	int _fd;
	uint8_t _buffer[256];
	int _buffer_data;
};

struct Size {
	Size ()
		: width (0)
		, height (0)
	{}
		
	Size (int w, int h)
		: width (w)
		, height (h)
	{}
	
	int width;
	int height;
};

class YUVImage {
public:
	YUVImage (int const *, Size, PixelFormat);
	YUVImage (uint8_t **, int const *, Size, PixelFormat);
	~YUVImage ();

	Size size () {
		return _size;
	}

	uint8_t** data () const {
		return _data;
	}
	
	uint8_t* data (int n) const {
		return _data[n];
	}

	int* line_size () const {
		return _line_size;
	}
	
	int line_size (int n) const {
		return _line_size[n];
	}
	
	PixelFormat pixel_format () const {
		return _pixel_format;
	}

	boost::shared_ptr<YUVImage> deep_copy ();

	static int const components;

private:
	
	uint8_t** _data;
	int* _line_size;
	Size _size;
	PixelFormat _pixel_format;
	bool _our_data;
};

#endif
