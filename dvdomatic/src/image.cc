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

#include <sstream>
#include <iomanip>
#include <iostream>
#include <execinfo.h>
#include <cxxabi.h>
#include <sys/time.h>
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
#include <libavutil/pixfmt.h>
}
#include "image.h"
#include "exceptions.h"
#include "scaler.h"

#ifdef DEBUG_HASH
#include <mhash.h>
#endif

using namespace std;
using namespace boost;

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
	default:
		assert (false);
	}

	return 0;
}

int
Image::components () const
{
	switch (_pixel_format) {
	case PIX_FMT_YUV420P:
		return 3;
	default:
		assert (false);
	}

	return 0;
}

#ifdef DEBUG_HASH
void
Image::hash () const
{
	MHASH ht = mhash_init (MHASH_MD5);
	if (ht == MHASH_FAILED) {
		throw EncodeError ("could not create hash thread");
	}
	
	for (int i = 0; i < components(); ++i) {
		mhash (ht, data()[i], line_size()[i] * lines(i));
	}
	
	uint8_t hash[16];
	mhash_deinit (ht, hash);
	
	printf ("YUV input: ");
	for (int i = 0; i < int (mhash_get_block_size (MHASH_MD5)); ++i) {
		printf ("%.2x", hash[i]);
	}
	printf ("\n");
}
#endif

/** Scale this image to a given size and convert it to RGB.
 *  Caller must pass both returned values to av_free ().
 *  @param out_size Output image size in pixels.
 *  @param scaler Scaler to use.
 */

pair<AVFrame *, uint8_t *>
Image::scale_and_convert_to_rgb (Size out_size, Scaler const * scaler) const
{
	assert (scaler);
	
	AVFrame* frame_out = avcodec_alloc_frame ();
	if (frame_out == 0) {
		throw EncodeError ("could not allocate frame");
	}

	struct SwsContext* scale_context = sws_getContext (
		size().width, size().height, pixel_format(),
		out_size.width, out_size.height, PIX_FMT_RGB24,
		scaler->ffmpeg_id (), 0, 0, 0
		);

	uint8_t* rgb = (uint8_t *) av_malloc (out_size.width * out_size.height * 3);
	avpicture_fill ((AVPicture *) frame_out, rgb, PIX_FMT_RGB24, out_size.width, out_size.height);
	
	/* Scale and convert from YUV to RGB */
	sws_scale (
		scale_context,
		data(), line_size(),
		0, size().height,
		frame_out->data, frame_out->linesize
		);

	sws_freeContext (scale_context);

	return make_pair (frame_out, rgb);
}

AllocImage::AllocImage (PixelFormat p, Size s)
	: Image (p)
	, _size (s)
{
	_data = (uint8_t **) av_malloc (components() * sizeof (uint8_t *));
	_line_size = (int *) av_malloc (components() * sizeof (int));
	
	for (int i = 0; i < components(); ++i) {
		_data[i] = 0;
		_line_size[i] = 0;
	}
}

AllocImage::~AllocImage ()
{
	for (int i = 0; i < components(); ++i) {
		av_free (_data[i]);
	}

	av_free (_data);
	av_free (_line_size);
}

void
AllocImage::set_line_size (int i, int s)
{
	_line_size[i] = s;
	_data[i] = (uint8_t *) av_malloc (s * lines (i));
}

uint8_t **
AllocImage::data () const
{
	return _data;
}

int *
AllocImage::line_size () const
{
	return _line_size;
}

Size
AllocImage::size () const
{
	return _size;
}


FilterBuffer::FilterBuffer (PixelFormat p, AVFilterBufferRef* b)
	: Image (p)
	, _buffer (b)
{

}

FilterBuffer::~FilterBuffer ()
{
	avfilter_unref_buffer (_buffer);
}

uint8_t **
FilterBuffer::data () const
{
	return _buffer->data;
}

int *
FilterBuffer::line_size () const
{
	return _buffer->linesize;
}

Size
FilterBuffer::size () const
{
	return Size (_buffer->video->w, _buffer->video->h);
}

