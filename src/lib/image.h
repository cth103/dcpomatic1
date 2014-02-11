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

/** @file src/image.h
 *  @brief A set of classes to describe video images.
 */

#ifndef DCPOMATIC_IMAGE_H
#define DCPOMATIC_IMAGE_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
}
#include <libdcp/image.h>
#include "util.h"
#include "position.h"

class Scaler;

class Image : public dcp::Image
{
public:
	Image (AVPixelFormat, dcp::Size, bool);
	Image (AVFrame *);
	Image (Image const &);
	Image (boost::shared_ptr<const Image>, bool);
	Image& operator= (Image const &);
	~Image ();
	
	uint8_t ** data () const;
	int * line_size () const;
	int * stride () const;
	dcp::Size size () const;
	bool aligned () const;

	int components () const;
	int line_factor (int) const;
	int lines (int) const;

	boost::shared_ptr<Image> scale (dcp::Size, Scaler const *, AVPixelFormat, bool aligned) const;
	boost::shared_ptr<Image> post_process (std::string, bool aligned) const;
	boost::shared_ptr<Image> crop (Crop c, bool aligned) const;

	boost::shared_ptr<Image> crop_scale_window (Crop c, dcp::Size, dcp::Size, Scaler const *, AVPixelFormat, bool aligned) const;
	
	void make_black ();
	void alpha_blend (boost::shared_ptr<const Image> image, Position<int> pos);
	void copy (boost::shared_ptr<const Image> image, Position<int> pos);

	void read_from_socket (boost::shared_ptr<Socket>);
	void write_to_socket (boost::shared_ptr<Socket>) const;
	
	AVPixelFormat pixel_format () const {
		return _pixel_format;
	}

private:
	friend class pixel_formats_test;
	
	void allocate ();
	void swap (Image &);
	float bytes_per_pixel (int) const;
	void yuv_16_black (uint16_t, bool);
	static uint16_t swap_16 (uint16_t);
	
	AVPixelFormat _pixel_format; ///< FFmpeg's way of describing the pixel format of this Image
	uint8_t** _data; ///< array of pointers to components
	int* _line_size; ///< array of sizes of the data in each line, in pixels (without any alignment padding bytes)
	int* _stride; ///< array of strides for each line (including any alignment padding bytes)
	bool _aligned;
};

#endif
