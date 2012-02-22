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

#ifndef DVDOMATIC_IMAGE_H
#define DVDOMATIC_IMAGE_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <gtkmm.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
}
#include "util.h"

class Scaler;

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

class FilterBufferImage : public Image
{
public:
	FilterBufferImage (PixelFormat, AVFilterBufferRef *);
	~FilterBufferImage ();

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

#endif
