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

#include <boost/test/unit_test.hpp>
#include <Magick++.h>
#include "lib/image.h"
#include "lib/scaler.h"

using std::string;
using std::cout;
using boost::shared_ptr;

BOOST_AUTO_TEST_CASE (aligned_image_test)
{
	Image* s = new Image (PIX_FMT_RGB24, libdcp::Size (50, 50), true);
	BOOST_CHECK_EQUAL (s->components(), 1);
	/* 160 is 150 aligned to the nearest 32 bytes */
	BOOST_CHECK_EQUAL (s->stride()[0], 160);
	BOOST_CHECK_EQUAL (s->line_size()[0], 150);
	BOOST_CHECK (s->data()[0]);
	BOOST_CHECK (!s->data()[1]);
	BOOST_CHECK (!s->data()[2]);
	BOOST_CHECK (!s->data()[3]);

	/* copy constructor */
	Image* t = new Image (*s);
	BOOST_CHECK_EQUAL (t->components(), 1);
	BOOST_CHECK_EQUAL (t->stride()[0], 160);
	BOOST_CHECK_EQUAL (t->line_size()[0], 150);
	BOOST_CHECK (t->data()[0]);
	BOOST_CHECK (!t->data()[1]);
	BOOST_CHECK (!t->data()[2]);
	BOOST_CHECK (!t->data()[3]);
	BOOST_CHECK (t->data() != s->data());
	BOOST_CHECK (t->data()[0] != s->data()[0]);
	BOOST_CHECK (t->line_size() != s->line_size());
	BOOST_CHECK (t->line_size()[0] == s->line_size()[0]);
	BOOST_CHECK (t->stride() != s->stride());
	BOOST_CHECK (t->stride()[0] == s->stride()[0]);

	/* assignment operator */
	Image* u = new Image (PIX_FMT_YUV422P, libdcp::Size (150, 150), false);
	*u = *s;
	BOOST_CHECK_EQUAL (u->components(), 1);
	BOOST_CHECK_EQUAL (u->stride()[0], 160);
	BOOST_CHECK_EQUAL (u->line_size()[0], 150);
	BOOST_CHECK (u->data()[0]);
	BOOST_CHECK (!u->data()[1]);
	BOOST_CHECK (!u->data()[2]);
	BOOST_CHECK (!u->data()[3]);
	BOOST_CHECK (u->data() != s->data());
	BOOST_CHECK (u->data()[0] != s->data()[0]);
	BOOST_CHECK (u->line_size() != s->line_size());
	BOOST_CHECK (u->line_size()[0] == s->line_size()[0]);
	BOOST_CHECK (u->stride() != s->stride());
	BOOST_CHECK (u->stride()[0] == s->stride()[0]);

	delete s;
	delete t;
	delete u;
}

BOOST_AUTO_TEST_CASE (compact_image_test)
{
	Image* s = new Image (PIX_FMT_RGB24, libdcp::Size (50, 50), false);
	BOOST_CHECK_EQUAL (s->components(), 1);
	BOOST_CHECK_EQUAL (s->stride()[0], 50 * 3);
	BOOST_CHECK_EQUAL (s->line_size()[0], 50 * 3);
	BOOST_CHECK (s->data()[0]);
	BOOST_CHECK (!s->data()[1]);
	BOOST_CHECK (!s->data()[2]);
	BOOST_CHECK (!s->data()[3]);

	/* copy constructor */
	Image* t = new Image (*s);
	BOOST_CHECK_EQUAL (t->components(), 1);
	BOOST_CHECK_EQUAL (t->stride()[0], 50 * 3);
	BOOST_CHECK_EQUAL (t->line_size()[0], 50 * 3);
	BOOST_CHECK (t->data()[0]);
	BOOST_CHECK (!t->data()[1]);
	BOOST_CHECK (!t->data()[2]);
	BOOST_CHECK (!t->data()[3]);
	BOOST_CHECK (t->data() != s->data());
	BOOST_CHECK (t->data()[0] != s->data()[0]);
	BOOST_CHECK (t->line_size() != s->line_size());
	BOOST_CHECK (t->line_size()[0] == s->line_size()[0]);
	BOOST_CHECK (t->stride() != s->stride());
	BOOST_CHECK (t->stride()[0] == s->stride()[0]);

	/* assignment operator */
	Image* u = new Image (PIX_FMT_YUV422P, libdcp::Size (150, 150), true);
	*u = *s;
	BOOST_CHECK_EQUAL (u->components(), 1);
	BOOST_CHECK_EQUAL (u->stride()[0], 50 * 3);
	BOOST_CHECK_EQUAL (u->line_size()[0], 50 * 3);
	BOOST_CHECK (u->data()[0]);
	BOOST_CHECK (!u->data()[1]);
	BOOST_CHECK (!u->data()[2]);
	BOOST_CHECK (!u->data()[3]);
	BOOST_CHECK (u->data() != s->data());
	BOOST_CHECK (u->data()[0] != s->data()[0]);
	BOOST_CHECK (u->line_size() != s->line_size());
	BOOST_CHECK (u->line_size()[0] == s->line_size()[0]);
	BOOST_CHECK (u->stride() != s->stride());
	BOOST_CHECK (u->stride()[0] == s->stride()[0]);

	delete s;
	delete t;
	delete u;
}

static
boost::shared_ptr<Image>
read_file (string file)
{
	Magick::Image magick_image (file.c_str ());
	libdcp::Size size (magick_image.columns(), magick_image.rows());

	boost::shared_ptr<Image> image (new Image (PIX_FMT_RGB24, size, true));

#ifdef DCPOMATIC_IMAGE_MAGICK
	using namespace MagickCore;
#endif

	uint8_t* p = image->data()[0];
	for (int y = 0; y < size.height; ++y) {
		uint8_t* q = p;
		for (int x = 0; x < size.width; ++x) {
			Magick::Color c = magick_image.pixelColor (x, y);
#ifdef DCPOMATIC_IMAGE_MAGICK
			*q++ = c.redQuantum() * 255 / QuantumRange;
			*q++ = c.greenQuantum() * 255 / QuantumRange;
			*q++ = c.blueQuantum() * 255 / QuantumRange;
#else
			*q++ = c.redQuantum() * 255 / MaxRGB;
			*q++ = c.greenQuantum() * 255 / MaxRGB;
			*q++ = c.blueQuantum() * 255 / MaxRGB;
#endif
		}
		p += image->stride()[0];
	}

	return image;
}

static
void
write_file (shared_ptr<Image> image, string file)
{
#ifdef DCPOMATIC_IMAGE_MAGICK
	using namespace MagickCore;
#endif

	Magick::Image magick_image (Magick::Geometry (image->size().width, image->size().height), Magick::Color (0, 0, 0));
	uint8_t*p = image->data()[0];
	for (int y = 0; y < image->size().height; ++y) {
		uint8_t* q = p;
		for (int x = 0; x < image->size().width; ++x) {
#ifdef DCPOMATIC_IMAGE_MAGICK
			Magick::Color c (q[0] * QuantumRange / 256, q[1] * QuantumRange / 256, q[2] * QuantumRange / 256);
#else
			Magick::Color c (q[0] * MaxRGB / 256, q[1] * MaxRGB / 256, q[2] * MaxRGB / 256);
#endif
			magick_image.pixelColor (x, y, c);
			q += 3;
		}
		p += image->stride()[0];
	}

	magick_image.write (file.c_str ());
}
