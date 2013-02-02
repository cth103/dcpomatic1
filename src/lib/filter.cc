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

/** @file src/filter.cc
 *  @brief A class to describe one of FFmpeg's video or post-processing filters.
 */

#include "filter.h"
extern "C" {
#include <libavfilter/avfilter.h>
#include <libpostproc/postprocess.h>
}

using namespace std;

vector<Filter const *> Filter::_filters;

/** @param i Our id.
 *  @param n User-visible name.
 *  @param c User-visible category.
 *  @param v String for a FFmpeg video filter descriptor, or "".
 *  @param p String for a FFmpeg post-processing descriptor, or "".
 */
Filter::Filter (string i, string n, string c, string v, string p)
	: _id (i)
	, _name (n)
	, _category (c)
	, _vf (v)
	, _pp (p)
{

}

/** @return All available filters */
vector<Filter const *>
Filter::all ()
{
	return _filters;
}


/** Set up the static _filters vector; must be called before from_*
 *  methods are used.
 */
void
Filter::setup_filters ()
{
	/* Note: "none" is a magic id name, so don't use it here */
	   
	maybe_add ("pphb",      "Horizontal deblocking filter",                "De-blocking",     "",          "hb");
	maybe_add ("ppvb",      "Vertical deblocking filter",                  "De-blocking",     "",          "vb");
	maybe_add ("ppha",      "Horizontal deblocking filter A",              "De-blocking",     "",          "ha");
	maybe_add ("ppva",      "Vertical deblocking filter A",                "De-blocking",     "",          "va");
	maybe_add ("pph1",      "Experimental horizontal deblocking filter 1", "De-blocking",     "",          "h1");
	maybe_add ("pphv",      "Experimental vertical deblocking filter 1",   "De-blocking",     "",          "v1");
	maybe_add ("ppdr",      "Deringing filter",                            "Misc",            "",          "dr");
	maybe_add ("pplb",      "Linear blend deinterlacer",                   "De-interlacing",  "",          "lb");
	maybe_add ("ppli",      "Linear interpolating deinterlacer",           "De-interlacing",  "",          "li");
	maybe_add ("ppci",      "Cubic interpolating deinterlacer",            "De-interlacing",  "",          "ci");
	maybe_add ("ppmd",      "Median deinterlacer",                         "De-interlacing",  "",          "md");
	maybe_add ("ppfd",      "FFMPEG deinterlacer",                         "De-interlacing",  "",          "fd");
	maybe_add ("ppl5",      "FIR low-pass deinterlacer",                   "De-interlacing",  "",          "l5");
	maybe_add ("mcdeint",   "Motion compensating deinterlacer",            "De-interlacing",  "mcdeint",   "");
	maybe_add ("kerndeint", "Kernel deinterlacer",                         "De-interlacing",  "kerndeint", "");
	maybe_add ("yadif",     "Yet Another Deinterlacing Filter",            "De-interlacing",  "yadif",     "");
	maybe_add ("pptn",      "Temporal noise reducer",                      "Noise reduction", "",          "tn");
	maybe_add ("ppfq",      "Force quantizer",                             "Misc",            "",          "fq");
	maybe_add ("gradfun",   "Gradient debander",                           "Misc",            "gradfun",   "");
	maybe_add ("unsharp",   "Unsharp mask and Gaussian blur",              "Misc",            "unsharp",   "");
	maybe_add ("denoise3d", "3D denoiser",                                 "Noise reduction", "denoise3d", "");
	maybe_add ("hqdn3d",    "High quality 3D denoiser",                    "Noise reduction", "hqdn3d",    "");
	maybe_add ("telecine",  "Telecine filter",                             "Misc",            "telecine",  "");
	maybe_add ("ow",        "Overcomplete wavelet denoiser",               "Noise reduction", "mp=ow",     "");
}

void
Filter::maybe_add (string i, string n, string c, string v, string p)
{
	if (!v.empty ()) {
		if (avfilter_get_by_name (i.c_str())) {
			_filters.push_back (new Filter (i, n, c, v, p));
		}
	} else if (!p.empty ()) {
		pp_mode* m = pp_get_mode_by_name_and_quality (p.c_str(), PP_QUALITY_MAX);
		if (m) {
			_filters.push_back (new Filter (i, n, c, v, p));
			pp_free_mode (m);
		}
	}
}

/** @param filters Set of filters.
 *  @return A pair; .first is a string to pass to FFmpeg for the video filters,
 *  .second is a string to pass for the post-processors.
 */
pair<string, string>
Filter::ffmpeg_strings (vector<Filter const *> const & filters)
{
	string vf;
	string pp;

	for (vector<Filter const *>::const_iterator i = filters.begin(); i != filters.end(); ++i) {
		if (!(*i)->vf().empty ()) {
			if (!vf.empty ()) {
				vf += ",";
			}
			vf += (*i)->vf ();
		}
		
		if (!(*i)->pp().empty ()) {
			if (!pp.empty()) {
				pp += ",";
			}
			pp += (*i)->pp ();
		}
	}

	return make_pair (vf, pp);
}

/** @param d Our id.
 *  @return Corresponding Filter, or 0.
 */
Filter const *
Filter::from_id (string d)
{
	vector<Filter const *>::iterator i = _filters.begin ();
	while (i != _filters.end() && (*i)->id() != d) {
		++i;
	}

	if (i == _filters.end ()) {
		return 0;
	}

	return *i;
}

