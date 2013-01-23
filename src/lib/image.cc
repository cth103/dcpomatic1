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

/** @file src/image.cc
 *  @brief A set of classes to describe video images.
 */

#include <sstream>
#include <iomanip>
#include <iostream>
#include <sys/time.h>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <openjpeg.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfiltergraph.h>
#include <libpostproc/postprocess.h>
#include <libavutil/pixfmt.h>
}
#include "image.h"
#include "exceptions.h"
#include "scaler.h"

using namespace std;
using namespace boost;
using libdcp::Size;

void
Image::swap (Image& other)
{
	std::swap (_pixel_format, other._pixel_format);
}

/** @param n Component index.
 *  @return Number of lines in the image for the given component.
 */
int
Image::lines (int n) const
{
	switch (_pixel_format) {
	case PIX_FMT_YUV420P:
		if (n == 0) {
			return size().height;
		} else {
			return size().height / 2;
		}
		break;
	case PIX_FMT_RGB24:
	case PIX_FMT_RGBA:
	case PIX_FMT_YUV422P10LE:
	case PIX_FMT_YUV422P:
		return size().height;
	default:
		assert (false);
	}

	return 0;
}

/** @return Number of components */
int
Image::components () const
{
	switch (_pixel_format) {
	case PIX_FMT_YUV420P:
	case PIX_FMT_YUV422P10LE:
	case PIX_FMT_YUV422P:
		return 3;
	case PIX_FMT_RGB24:
	case PIX_FMT_RGBA:
		return 1;
	default:
		assert (false);
	}

	return 0;
}

shared_ptr<Image>
Image::scale (libdcp::Size out_size, Scaler const * scaler, bool result_aligned) const
{
	assert (scaler);
	/* Empirical testing suggests that sws_scale() will crash if
	   the input image is not aligned.
	*/
	assert (aligned ());

	shared_ptr<Image> scaled (new SimpleImage (pixel_format(), out_size, result_aligned));

	struct SwsContext* scale_context = sws_getContext (
		size().width, size().height, pixel_format(),
		out_size.width, out_size.height, pixel_format(),
		scaler->ffmpeg_id (), 0, 0, 0
		);

	sws_scale (
		scale_context,
		data(), stride(),
		0, size().height,
		scaled->data(), scaled->stride()
		);

	sws_freeContext (scale_context);

	return scaled;
}

/** Scale this image to a given size and convert it to RGB.
 *  @param out_size Output image size in pixels.
 *  @param scaler Scaler to use.
 */
shared_ptr<Image>
Image::scale_and_convert_to_rgb (libdcp::Size out_size, int padding, Scaler const * scaler, bool result_aligned) const
{
	assert (scaler);
	/* Empirical testing suggests that sws_scale() will crash if
	   the input image is not aligned.
	*/
	assert (aligned ());

	libdcp::Size content_size = out_size;
	content_size.width -= (padding * 2);

	shared_ptr<Image> rgb (new SimpleImage (PIX_FMT_RGB24, content_size, result_aligned));

	struct SwsContext* scale_context = sws_getContext (
		size().width, size().height, pixel_format(),
		content_size.width, content_size.height, PIX_FMT_RGB24,
		scaler->ffmpeg_id (), 0, 0, 0
		);

	/* Scale and convert to RGB from whatever its currently in (which may be RGB) */
	sws_scale (
		scale_context,
		data(), stride(),
		0, size().height,
		rgb->data(), rgb->stride()
		);

	/* Put the image in the right place in a black frame if are padding; this is
	   a bit grubby and expensive, but probably inconsequential in the great
	   scheme of things.
	*/
	if (padding > 0) {
		shared_ptr<Image> padded_rgb (new SimpleImage (PIX_FMT_RGB24, out_size, result_aligned));
		padded_rgb->make_black ();

		/* XXX: we are cheating a bit here; we know the frame is RGB so we can
		   make assumptions about its composition.
		*/
		uint8_t* p = padded_rgb->data()[0] + padding * 3;
		uint8_t* q = rgb->data()[0];
		for (int j = 0; j < rgb->lines(0); ++j) {
			memcpy (p, q, rgb->line_size()[0]);
			p += padded_rgb->stride()[0];
			q += rgb->stride()[0];
		}

		rgb = padded_rgb;
	}

	sws_freeContext (scale_context);

	return rgb;
}

/** Run a FFmpeg post-process on this image and return the processed version.
 *  @param pp Flags for the required set of post processes.
 *  @return Post-processed image.
 */
shared_ptr<Image>
Image::post_process (string pp, bool aligned) const
{
	shared_ptr<Image> out (new SimpleImage (pixel_format(), size (), aligned));

	int pp_format = 0;
	switch (pixel_format()) {
	case PIX_FMT_YUV420P:
		pp_format = PP_FORMAT_420;
		break;
	case PIX_FMT_YUV422P10LE:
	case PIX_FMT_YUV422P:
		pp_format = PP_FORMAT_422;
		break;
	default:
		assert (false);
	}
		
	pp_mode* mode = pp_get_mode_by_name_and_quality (pp.c_str (), PP_QUALITY_MAX);
	pp_context* context = pp_get_context (size().width, size().height, pp_format | PP_CPU_CAPS_MMX2);

	pp_postprocess (
		(const uint8_t **) data(), stride(),
		out->data(), out->stride(),
		size().width, size().height,
		0, 0, mode, context, 0
		);
		
	pp_free_mode (mode);
	pp_free_context (context);

	return out;
}

shared_ptr<Image>
Image::crop (Crop crop, bool aligned) const
{
	libdcp::Size cropped_size = size ();
	cropped_size.width -= crop.left + crop.right;
	cropped_size.height -= crop.top + crop.bottom;

	shared_ptr<Image> out (new SimpleImage (pixel_format(), cropped_size, aligned));

	for (int c = 0; c < components(); ++c) {
		int const crop_left_in_bytes = bytes_per_pixel(c) * crop.left;
		int const cropped_width_in_bytes = bytes_per_pixel(c) * cropped_size.width;
			
		/* Start of the source line, cropped from the top but not the left */
		uint8_t* in_p = data()[c] + crop.top * stride()[c];
		uint8_t* out_p = out->data()[c];
		
		for (int y = 0; y < cropped_size.height; ++y) {
			memcpy (out_p, in_p + crop_left_in_bytes, cropped_width_in_bytes);
			in_p += line_size()[c];
			out_p += out->line_size()[c];
		}
	}

	return out;
}

void
Image::make_black ()
{
	switch (_pixel_format) {
	case PIX_FMT_YUV420P:
	case PIX_FMT_YUV422P:
		memset (data()[0], 0, lines(0) * stride()[0]);
		memset (data()[1], 0x7f, lines(1) * stride()[1]);
		memset (data()[2], 0x7f, lines(2) * stride()[2]);
		break;

	case PIX_FMT_YUV422P10LE:
		memset (data()[0], 0, lines(0) * stride()[0]);
		for (int i = 1; i < 3; ++i) {
			int16_t* p = reinterpret_cast<int16_t*> (data()[i]);
			for (int y = 0; y < size().height; ++y) {
				for (int x = 0; x < line_size()[i] / 2; ++x) {
					p[x] = (1 << 9) - 1;
				}
				p += stride()[i] / 2;
			}
		}
		break;
		
	case PIX_FMT_RGB24:		
		memset (data()[0], 0, lines(0) * stride()[0]);
		break;

	default:
		assert (false);
	}
}

void
Image::alpha_blend (shared_ptr<const Image> other, Position position)
{
	/* Only implemented for RGBA onto RGB24 so far */
	assert (_pixel_format == PIX_FMT_RGB24 && other->pixel_format() == PIX_FMT_RGBA);

	int start_tx = position.x;
	int start_ox = 0;

	if (start_tx < 0) {
		start_ox = -start_tx;
		start_tx = 0;
	}

	int start_ty = position.y;
	int start_oy = 0;

	if (start_ty < 0) {
		start_oy = -start_ty;
		start_ty = 0;
	}

	for (int ty = start_ty, oy = start_oy; ty < size().height && oy < other->size().height; ++ty, ++oy) {
		uint8_t* tp = data()[0] + ty * stride()[0] + position.x * 3;
		uint8_t* op = other->data()[0] + oy * other->stride()[0];
		for (int tx = start_tx, ox = start_ox; tx < size().width && ox < other->size().width; ++tx, ++ox) {
			float const alpha = float (op[3]) / 255;
			tp[0] = (tp[0] * (1 - alpha)) + op[0] * alpha;
			tp[1] = (tp[1] * (1 - alpha)) + op[1] * alpha;
			tp[2] = (tp[2] * (1 - alpha)) + op[2] * alpha;
			tp += 3;
			op += 4;
		}
	}
}

void
Image::read_from_socket (shared_ptr<Socket> socket)
{
	for (int i = 0; i < components(); ++i) {
		uint8_t* p = data()[i];
		for (int y = 0; y < lines(i); ++y) {
			socket->read_definite_and_consume (p, line_size()[i], 30);
			p += stride()[i];
		}
	}
}

void
Image::write_to_socket (shared_ptr<Socket> socket) const
{
	for (int i = 0; i < components(); ++i) {
		uint8_t* p = data()[i];
		for (int y = 0; y < lines(i); ++y) {
			socket->write (p, line_size()[i], 30);
			p += stride()[i];
		}
	}
}


float
Image::bytes_per_pixel (int c) const
{
	if (c == 3) {
		return 0;
	}
	
	switch (_pixel_format) {
	case PIX_FMT_RGB24:
		if (c == 0) {
			return 3;
		} else {
			return 0;
		}
	case PIX_FMT_RGBA:
		if (c == 0) {
			return 4;
		} else {
			return 0;
		}
	case PIX_FMT_YUV420P:
	case PIX_FMT_YUV422P:
		if (c == 0) {
			return 1;
		} else {
			return 0.5;
		}
	case PIX_FMT_YUV422P10LE:
		if (c == 0) {
			return 2;
		} else {
			return 1;
		}
	default:
		assert (false);
	}

	return 0;
}


/** Construct a SimpleImage of a given size and format, allocating memory
 *  as required.
 *
 *  @param p Pixel format.
 *  @param s Size in pixels.
 */
SimpleImage::SimpleImage (AVPixelFormat p, libdcp::Size s, bool aligned)
	: Image (p)
	, _size (s)
	, _aligned (aligned)
{
	allocate ();
}

void
SimpleImage::allocate ()
{
	_data = (uint8_t **) av_malloc (4 * sizeof (uint8_t *));
	_data[0] = _data[1] = _data[2] = _data[3] = 0;
	
	_line_size = (int *) av_malloc (4 * sizeof (int));
	_line_size[0] = _line_size[1] = _line_size[2] = _line_size[3] = 0;
	
	_stride = (int *) av_malloc (4 * sizeof (int));
	_stride[0] = _stride[1] = _stride[2] = _stride[3] = 0;

	for (int i = 0; i < components(); ++i) {
		_line_size[i] = _size.width * bytes_per_pixel(i);
		_stride[i] = stride_round_up (i, _line_size, _aligned ? 32 : 1);
		_data[i] = (uint8_t *) av_malloc (_stride[i] * lines (i));
	}
}

SimpleImage::SimpleImage (SimpleImage const & other)
	: Image (other)
{
	_size = other._size;
	_aligned = other._aligned;
	
	allocate ();

	for (int i = 0; i < components(); ++i) {
		memcpy (_data[i], other._data[i], _line_size[i] * lines(i));
	}
}

SimpleImage&
SimpleImage::operator= (SimpleImage const & other)
{
	if (this == &other) {
		return *this;
	}

	SimpleImage tmp (other);
	swap (tmp);
	return *this;
}

void
SimpleImage::swap (SimpleImage & other)
{
	Image::swap (other);
	
	std::swap (_size, other._size);

	for (int i = 0; i < 4; ++i) {
		std::swap (_data[i], other._data[i]);
		std::swap (_line_size[i], other._line_size[i]);
		std::swap (_stride[i], other._stride[i]);
	}

	std::swap (_aligned, other._aligned);
}

/** Destroy a SimpleImage */
SimpleImage::~SimpleImage ()
{
	for (int i = 0; i < components(); ++i) {
		av_free (_data[i]);
	}

	av_free (_data);
	av_free (_line_size);
	av_free (_stride);
}

uint8_t **
SimpleImage::data () const
{
	return _data;
}

int *
SimpleImage::line_size () const
{
	return _line_size;
}

int *
SimpleImage::stride () const
{
	return _stride;
}

libdcp::Size
SimpleImage::size () const
{
	return _size;
}

bool
SimpleImage::aligned () const
{
	return _aligned;
}

FilterBufferImage::FilterBufferImage (AVPixelFormat p, AVFilterBufferRef* b)
	: Image (p)
	, _buffer (b)
{

}

FilterBufferImage::~FilterBufferImage ()
{
	avfilter_unref_buffer (_buffer);
}

uint8_t **
FilterBufferImage::data () const
{
	return _buffer->data;
}

int *
FilterBufferImage::line_size () const
{
	return _buffer->linesize;
}

int *
FilterBufferImage::stride () const
{
	/* XXX? */
	return _buffer->linesize;
}

libdcp::Size
FilterBufferImage::size () const
{
	return libdcp::Size (_buffer->video->w, _buffer->video->h);
}

bool
FilterBufferImage::aligned () const
{
	/* XXX? */
	return true;
}

RGBPlusAlphaImage::RGBPlusAlphaImage (shared_ptr<const Image> im)
	: SimpleImage (im->pixel_format(), im->size(), false)
{
	assert (im->pixel_format() == PIX_FMT_RGBA);

	_alpha = (uint8_t *) av_malloc (im->size().width * im->size().height);

	uint8_t* in = im->data()[0];
	uint8_t* out = data()[0];
	uint8_t* out_alpha = _alpha;
	for (int y = 0; y < im->size().height; ++y) {
		uint8_t* in_r = in;
		for (int x = 0; x < im->size().width; ++x) {
			*out++ = *in_r++;
			*out++ = *in_r++;
			*out++ = *in_r++;
			*out_alpha++ = *in_r++;
		}

		in += im->stride()[0];
	}
}

RGBPlusAlphaImage::~RGBPlusAlphaImage ()
{
	av_free (_alpha);
}

