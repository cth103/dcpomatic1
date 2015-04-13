/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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
#include <libdcp/colour_matrix.h>
#include "lib/colour_conversion.h"

using std::cout;

/* Basic test of identifier() for ColourConversion (i.e. a hash of the numbers) */
BOOST_AUTO_TEST_CASE (colour_conversion_test)
{
	ColourConversion A (
		2.4, true, YUV_TO_RGB_REC601,
		Chromaticity (0.64, 0.33), Chromaticity (0.3, 0.6), Chromaticity (0.15, 0.06), Chromaticity (0.3127, 0.329), 2.6
		);

	ColourConversion B (
		2.4, false, YUV_TO_RGB_REC601,
		Chromaticity (0.64, 0.33), Chromaticity (0.3, 0.6), Chromaticity (0.15, 0.06), Chromaticity (0.3127, 0.329), 2.6
		);

	BOOST_CHECK_EQUAL (A.identifier(), "ba02e01a7614382832db990b31ee6821");
	BOOST_CHECK_EQUAL (B.identifier(), "156fa7e280d565676a0bc971be0d94bc");
}

/* Check the calculation of conversion matrices */
BOOST_AUTO_TEST_CASE (colour_conversion_matrix_test)
{
	ColourConversion c;
	boost::numeric::ublas::matrix<double> m = c.rgb_to_xyz ();
	BOOST_CHECK_CLOSE (m(0, 0), 0.4123908, 0.01);
	BOOST_CHECK_CLOSE (m(0, 1), 0.3575843, 0.01);
	BOOST_CHECK_CLOSE (m(0, 2), 0.1804808, 0.01);
	BOOST_CHECK_CLOSE (m(1, 0), 0.2126390, 0.01);
	BOOST_CHECK_CLOSE (m(1, 1), 0.7151687, 0.01);
	BOOST_CHECK_CLOSE (m(1, 2), 0.0721923, 0.01);
	BOOST_CHECK_CLOSE (m(2, 0), 0.0193308, 0.01);
	BOOST_CHECK_CLOSE (m(2, 1), 0.1191948, 0.01);
	BOOST_CHECK_CLOSE (m(2, 2), 0.9505322, 0.01);
}
