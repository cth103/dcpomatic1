/*
    Copyright (C) 2013-2015 Carl Hetherington <cth@carlh.net>

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

#ifndef DCPOMATIC_COLOUR_CONVERSION_H
#define DCPOMATIC_COLOUR_CONVERSION_H

/* Hack for OS X compile failure; see https://bugs.launchpad.net/hugin/+bug/910160 */
#ifdef check
#undef check
#endif

#include <boost/utility.hpp>
#include <boost/optional.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <libcxml/cxml.h>

namespace xmlpp {
	class Node;
}

enum YUVToRGB {
	YUV_TO_RGB_REC601,
	YUV_TO_RGB_REC709,
	YUV_TO_RGB_COUNT
};

class Chromaticity
{
public:
	Chromaticity ()
		: x (0)
		, y (0)
	{}

	Chromaticity (double x_, double y_)
		: x (x_)
		, y (y_)
	{}
		
	double x;
	double y;

	double z () const {
		return 1 - x - y;
	}
};

class ColourConversion
{
public:
	ColourConversion ();
	ColourConversion (
		double, bool, YUVToRGB yuv_to_rgb_, Chromaticity red_, Chromaticity green_, Chromaticity blue_, Chromaticity white_, double
		);
	ColourConversion (cxml::NodePtr);

	virtual void as_xml (xmlpp::Node *) const;
	std::string identifier () const;

	boost::optional<size_t> preset () const;

	static boost::optional<ColourConversion> from_xml (cxml::NodePtr);

	double input_gamma;
	bool input_gamma_linearised;
	YUVToRGB yuv_to_rgb;
	Chromaticity red;
	Chromaticity green;
	Chromaticity blue;
	Chromaticity white;
	double output_gamma;

	boost::numeric::ublas::matrix<double> rgb_to_xyz () const;
};

class PresetColourConversion
{
public:
	PresetColourConversion ();
	PresetColourConversion (
		std::string, double, bool, YUVToRGB yuv_to_rgb_, Chromaticity red_, Chromaticity green_, Chromaticity blue_, Chromaticity white_, double
		);
	PresetColourConversion (cxml::NodePtr);

	void as_xml (xmlpp::Node *) const;

	std::string name;
	ColourConversion conversion;
};

bool operator== (ColourConversion const &, ColourConversion const &);
bool operator!= (ColourConversion const &, ColourConversion const &);
bool operator== (PresetColourConversion const &, PresetColourConversion const &);

#endif
