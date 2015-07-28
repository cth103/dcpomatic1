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

#include <boost/foreach.hpp>
#include <libxml++/libxml++.h>
#include <libdcp/colour_matrix.h>
#include <libcxml/cxml.h>
#include "raw_convert.h"
#include "config.h"
#include "colour_conversion.h"
#include "util.h"
#include "md5_digester.h"

#include "i18n.h"

using std::list;
using std::string;
using std::cout;
using std::vector;
using boost::shared_ptr;
using boost::optional;

vector<PresetColourConversion> PresetColourConversion::_presets;

ColourConversion::ColourConversion ()
	: input_gamma (2.4)
	, input_gamma_linearised (true)
	, yuv_to_rgb (YUV_TO_RGB_REC601)
	  /* sRGB and Rec709 chromaticities */
	, red (0.64, 0.33)
	, green (0.3, 0.6)
	, blue (0.15, 0.06)
	  /* D65 */
	, white (0.3127, 0.329)
	, output_gamma (2.6)
{

}

ColourConversion::ColourConversion (
	double i, bool il, YUVToRGB yuv_to_rgb_, Chromaticity red_, Chromaticity green_, Chromaticity blue_, Chromaticity white_, double o
	)
	: input_gamma (i)
	, input_gamma_linearised (il)
	, yuv_to_rgb (yuv_to_rgb_)
	, red (red_)
	, green (green_)
	, blue (blue_)
	, white (white_)
	, output_gamma (o)
{

}

ColourConversion::ColourConversion (cxml::NodePtr node)
{
	input_gamma = node->number_child<double> ("InputGamma");
	input_gamma_linearised = node->bool_child ("InputGammaLinearised");
	yuv_to_rgb = static_cast<YUVToRGB> (node->optional_number_child<int>("YUVToRGB").get_value_or (YUV_TO_RGB_REC601));

	list<cxml::NodePtr> m = node->node_children ("Matrix");
	if (!m.empty ()) {
		/* Read in old <Matrix> nodes and convert them to chromaticities */
		boost::numeric::ublas::matrix<double> C (3, 3);
		for (list<cxml::NodePtr>::iterator i = m.begin(); i != m.end(); ++i) {
			int const ti = (*i)->number_attribute<int> ("i");
			int const tj = (*i)->number_attribute<int> ("j");
			C(ti, tj) = raw_convert<double> ((*i)->content ());
		}

		double const rd = C(0, 0) + C(1, 0) + C(2, 0);
		red = Chromaticity (C(0, 0) / rd, C(1, 0) / rd);
		double const gd = C(0, 1) + C(1, 1) + C(2, 1);
		green = Chromaticity (C(0, 1) / gd, C(1, 1) / gd);
		double const bd = C(0, 2) + C(1, 2) + C(2, 2);
		blue = Chromaticity (C(0, 2) / bd, C(1, 2) / bd);
		double const wd = C(0, 0) + C(0, 1) + C(0, 2) + C(1, 0) + C(1, 1) + C(1, 2) + C(2, 0) + C(2, 1) + C(2, 2);
		white = Chromaticity ((C(0, 0) + C(0, 1) + C(0, 2)) / wd, (C(1, 0) + C(1, 1) + C(1, 2)) / wd);
	} else {
		/* New-style chromaticities */
		red = Chromaticity (node->number_child<double> ("RedX"), node->number_child<double> ("RedY"));
		green = Chromaticity (node->number_child<double> ("GreenX"), node->number_child<double> ("GreenY"));
		blue = Chromaticity (node->number_child<double> ("BlueX"), node->number_child<double> ("BlueY"));
		white = Chromaticity (node->number_child<double> ("WhiteX"), node->number_child<double> ("WhiteY"));
		if (node->optional_node_child ("AdjustedWhiteX")) {
			adjusted_white = Chromaticity (node->number_child<double> ("AdjustedWhiteX"), node->number_child<double> ("AdjustedWhiteY"));
		}
	}

	output_gamma = node->number_child<double> ("OutputGamma");
}

boost::optional<ColourConversion>
ColourConversion::from_xml (cxml::NodePtr node)
{
	if (!node->optional_node_child ("InputGamma")) {
		return boost::optional<ColourConversion> ();
	}

	return ColourConversion (node);
}

void
ColourConversion::as_xml (xmlpp::Node* node) const
{
	node->add_child("InputGamma")->add_child_text (raw_convert<string> (input_gamma));
	node->add_child("InputGammaLinearised")->add_child_text (input_gamma_linearised ? "1" : "0");
	node->add_child("YUVToRGB")->add_child_text (raw_convert<string> (yuv_to_rgb));

	node->add_child("RedX")->add_child_text (raw_convert<string> (red.x));
	node->add_child("RedY")->add_child_text (raw_convert<string> (red.y));
	node->add_child("GreenX")->add_child_text (raw_convert<string> (green.x));
	node->add_child("GreenY")->add_child_text (raw_convert<string> (green.y));
	node->add_child("BlueX")->add_child_text (raw_convert<string> (blue.x));
	node->add_child("BlueY")->add_child_text (raw_convert<string> (blue.y));
	node->add_child("WhiteX")->add_child_text (raw_convert<string> (white.x));
	node->add_child("WhiteY")->add_child_text (raw_convert<string> (white.y));
	if (adjusted_white) {
		node->add_child("AdjustedWhiteX")->add_child_text (raw_convert<string> (adjusted_white.get().x));
		node->add_child("AdjustedWhiteY")->add_child_text (raw_convert<string> (adjusted_white.get().y));
	}

	node->add_child("OutputGamma")->add_child_text (raw_convert<string> (output_gamma));
}

optional<size_t>
ColourConversion::preset () const
{
	vector<PresetColourConversion> presets = PresetColourConversion::all ();
	size_t i = 0;
	while (i < presets.size() && presets[i].conversion != *this) {
		++i;
	}

	if (i >= presets.size ()) {
		return optional<size_t> ();
	}

	return i;
}

string
ColourConversion::identifier () const
{
	MD5Digester digester;

	digester.add (input_gamma);
	digester.add (input_gamma_linearised);
	digester.add (yuv_to_rgb);

	digester.add (red.x);
	digester.add (red.y);
	digester.add (green.x);
	digester.add (green.y);
	digester.add (blue.x);
	digester.add (blue.y);
	digester.add (white.x);
	digester.add (white.y);

	if (adjusted_white) {
		digester.add (adjusted_white.get().x);
		digester.add (adjusted_white.get().y);
	}

	digester.add (output_gamma);

	return digester.get ();
}

boost::numeric::ublas::matrix<double>
ColourConversion::rgb_to_xyz () const
{
	/* See doc/design/colour.tex */

	double const D = (red.x - white.x) * (white.y - blue.y) - (white.x - blue.x) * (red.y - white.y);
	double const E = (white.x - green.x) * (red.y - white.y) - (red.x - white.x) * (white.y - green.y);
	double const F = (white.x - green.x) * (white.y - blue.y) - (white.x - blue.x) * (white.y - green.y);
	double const P = red.y + green.y * D / F + blue.y * E / F;

	boost::numeric::ublas::matrix<double> C (3, 3);
	C(0, 0) = red.x / P;
	C(0, 1) = green.x * D / (F * P);
	C(0, 2) = blue.x * E / (F * P);
	C(1, 0) = red.y / P;
	C(1, 1) = green.y * D / (F * P);
	C(1, 2) = blue.y * E / (F * P);
	C(2, 0) = red.z() / P;
	C(2, 1) = green.z() * D / (F * P);
	C(2, 2) = blue.z() * E / (F * P);
	return C;
}

boost::numeric::ublas::matrix<double>
ColourConversion::bradford () const
{
	if (!adjusted_white || fabs (adjusted_white.get().x) < 1e-6 || fabs (adjusted_white.get().y) < 1e-6) {
		boost::numeric::ublas::matrix<double> B = boost::numeric::ublas::zero_matrix<double> (3, 3);
		B(0, 0) = 1;
		B(1, 1) = 1;
		B(2, 2) = 1;
		return B;
	}

	/* See doc/design/colour.tex */

	boost::numeric::ublas::matrix<double> M (3, 3);
	M(0, 0) = 0.8951;
	M(0, 1) = 0.2664;
	M(0, 2) = -0.1614;
	M(1, 0) = -0.7502;
	M(1, 1) = 1.7135;
	M(1, 2) = 0.0367;
	M(2, 0) = 0.0389;
	M(2, 1) = -0.0685;
	M(2, 2) = 1.0296;

	boost::numeric::ublas::matrix<double> Mi (3, 3);
	Mi(0, 0) = 0.9869929055;
	Mi(0, 1) = -0.1470542564;
	Mi(0, 2) = 0.1599626517;
	Mi(1, 0) = 0.4323052697;
	Mi(1, 1) = 0.5183602715;
	Mi(1, 2) = 0.0492912282;
	Mi(2, 0) = -0.0085286646;
	Mi(2, 1) = 0.0400428217;
	Mi(2, 2) = 0.9684866958;

	boost::numeric::ublas::matrix<double> Gp (3, 1);
	Gp(0, 0) = white.x / white.y;
	Gp(1, 0) = 1;
	Gp(2, 0) = (1 - white.x - white.y) / white.y;

	boost::numeric::ublas::matrix<double> G = boost::numeric::ublas::prod (M, Gp);

	boost::numeric::ublas::matrix<double> Hp (3, 1);
	Hp(0, 0) = adjusted_white.get().x / adjusted_white.get().y;
	Hp(1, 0) = 1;
	Hp(2, 0) = (1 - adjusted_white.get().x - adjusted_white.get().y) / adjusted_white.get().y;

	boost::numeric::ublas::matrix<double> H = boost::numeric::ublas::prod (M, Hp);

	boost::numeric::ublas::matrix<double> C = boost::numeric::ublas::zero_matrix<double> (3, 3);
	C(0, 0) = H(0, 0) / G(0, 0);
	C(1, 1) = H(1, 0) / G(1, 0);
	C(2, 2) = H(2, 0) / G(2, 0);

	boost::numeric::ublas::matrix<double> CM = boost::numeric::ublas::prod (C, M);
	return boost::numeric::ublas::prod (Mi, CM);
}

PresetColourConversion::PresetColourConversion ()
	: name (_("Untitled"))
{

}

PresetColourConversion::PresetColourConversion (
	string name_, string id_, double i, bool il, YUVToRGB yuv_to_rgb, Chromaticity red, Chromaticity green, Chromaticity blue, Chromaticity white, double o
	)
	: name (name_)
	, id (id_)
	, conversion (i, il, yuv_to_rgb, red, green, blue, white, o)
{

}

static bool
about_equal (double a, double b)
{
	static const double eps = 1e-6;
	return fabs (a - b) < eps;
}

bool
operator== (ColourConversion const & a, ColourConversion const & b)
{
	if (
		!about_equal (a.input_gamma, b.input_gamma) ||
		a.input_gamma_linearised != b.input_gamma_linearised ||
		a.yuv_to_rgb != b.yuv_to_rgb ||
		!about_equal (a.red.x, b.red.x) ||
		!about_equal (a.red.y, b.red.y) ||
		!about_equal (a.green.x, b.green.x) ||
		!about_equal (a.green.y, b.green.y) ||
		!about_equal (a.blue.x, b.blue.x) ||
		!about_equal (a.blue.y, b.blue.y) ||
		!about_equal (a.white.x, b.white.x) ||
		!about_equal (a.white.y, b.white.y) ||
		!about_equal (a.output_gamma, b.output_gamma)) {
		return false;
	}

	if (!a.adjusted_white && !b.adjusted_white) {
		return true;
	}

	if (
		a.adjusted_white && b.adjusted_white &&
		about_equal (a.adjusted_white.get().x, b.adjusted_white.get().x) &&
		about_equal (a.adjusted_white.get().y, b.adjusted_white.get().y)
		) {
		return true;
	}

	/* Otherwise one has an adjusted white and the other hasn't, or they both have but different */
	return false;
}

bool
operator!= (ColourConversion const & a, ColourConversion const & b)
{
	return !(a == b);
}

bool
operator== (PresetColourConversion const & a, PresetColourConversion const & b)
{
	return a.name == b.name && a.conversion == b.conversion;
}

void
PresetColourConversion::setup_colour_conversion_presets ()
{
	_presets.push_back (
		PresetColourConversion (
			_("sRGB"), "srgb", 2.4, true, YUV_TO_RGB_REC601,
			Chromaticity (0.64, 0.33), Chromaticity (0.3, 0.6), Chromaticity (0.15, 0.06), Chromaticity (0.3127, 0.329), 2.6
			)
		);

	_presets.push_back (
		PresetColourConversion (
			_("Rec. 601"), "rec601", 2.2, false, YUV_TO_RGB_REC601,
			Chromaticity (0.63, 0.34), Chromaticity (0.31, 0.595), Chromaticity (0.155, 0.07), Chromaticity (0.3127, 0.329), 2.6
			)
		);

	_presets.push_back (
		PresetColourConversion (
			_("Rec. 709"), "rec709", 2.2, false, YUV_TO_RGB_REC709,
			Chromaticity (0.64, 0.33), Chromaticity (0.3, 0.6), Chromaticity (0.15, 0.06), Chromaticity (0.3127, 0.329), 2.6
			)
		);

	_presets.push_back (
		PresetColourConversion (
			_("P3"), "p3", 2.6, false, YUV_TO_RGB_REC709,
			Chromaticity (0.68, 0.32), Chromaticity (0.265, 0.69), Chromaticity (0.15, 0.06), Chromaticity (0.314, 0.351), 2.6
			)
		);
}

PresetColourConversion
PresetColourConversion::from_id (string s)
{
	BOOST_FOREACH (PresetColourConversion const & i, _presets) {
		if (i.id == s) {
			return i;
		}
	}

	DCPOMATIC_ASSERT (false);
}
