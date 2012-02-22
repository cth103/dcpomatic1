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
#include <libavfilter/avfilter.h>
}

class Scaler;

extern std::string seconds_to_hms (int);
extern std::string seconds_to_approximate_hms (int);
extern Gtk::Label & left_aligned_label (std::string);
extern void stacktrace (std::ostream &, int);
extern std::string audio_sample_format_to_string (AVSampleFormat);
extern AVSampleFormat audio_sample_format_from_string (std::string);
extern std::string dependency_version_summary ();
extern void fd_write (int, uint8_t const *, int);
extern double seconds (struct timeval);

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

struct Size
{
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

class Image
{
public:
	Image (PixelFormat p)
		: _pixel_format (p)
	{}
	
	virtual ~Image () {}
	virtual uint8_t ** data () const = 0;
	virtual int * line_size () const = 0;
	virtual Size size () const = 0;

	int components () const;
	int lines (int) const;
	std::pair<AVFrame *, uint8_t *> scale_and_convert_to_rgb (Size, Scaler const *) const;
#ifdef DEBUG_HASH	
	void hash () const;
#endif	
	
	PixelFormat pixel_format () const {
		return _pixel_format;
	}

private:
	PixelFormat _pixel_format;
};

class FilterBuffer : public Image
{
public:
	FilterBuffer (PixelFormat, AVFilterBufferRef *);
	~FilterBuffer ();

	uint8_t ** data () const;
	int * line_size () const;
	Size size () const;

private:
	AVFilterBufferRef* _buffer;
};

class AllocImage : public Image
{
public:
	AllocImage (PixelFormat, Size);
	~AllocImage ();

	uint8_t ** data () const;
	int * line_size () const;
	Size size () const;
	
	void set_line_size (int, int);

private:
	Size _size;
	uint8_t** _data;
	int* _line_size;
};

class PeriodTimer
{
public:
	PeriodTimer (std::string);
	~PeriodTimer ();
	
private:
	
	std::string _name;
	struct timeval _start;
};

class StateTimer
{
public:
	StateTimer (std::string, std::string);
	~StateTimer ();

	void set_state (std::string);

private:
	std::string _name;
	std::string _state;
	double _time;
	std::map<std::string, double> _totals;
};

#endif
