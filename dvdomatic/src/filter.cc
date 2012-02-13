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

#include "filter.h"

using namespace std;

vector<Filter*> Filter::_filters;

Filter::Filter (string const & i, string const & n, string const & v, string const & p)
	: _id (i)
	, _name (n)
	, _vf (v)
	, _pp (p)
{

}

vector<Filter*>
Filter::get_all ()
{
	return _filters;
}

void
Filter::setup_filters ()
{
	_filters.push_back (new Filter ("pphb", "Horizontal deblocking filter", "", "hb"));
	_filters.push_back (new Filter ("ppvb", "Vertical deblocking filter", "", "vb"));
	_filters.push_back (new Filter ("ppha", "Horizontal deblocking filter A", "", "ha"));
	_filters.push_back (new Filter ("ppva", "Vertical deblocking filter A", "", "va"));
	_filters.push_back (new Filter ("pph1", "Experimental horizontal deblocking filter 1", "", "h1"));
	_filters.push_back (new Filter ("pphv", "Experimental vertical deblocking filter 1", "", "v1"));
	_filters.push_back (new Filter ("ppdr", "Deringing filter", "", "dr"));
	_filters.push_back (new Filter ("pplb", "Linear blend deinterlacer", "", "lb"));
	_filters.push_back (new Filter ("ppli", "Linear interpolating deinterlacer", "", "li"));
	_filters.push_back (new Filter ("ppci", "Cubic interpolating deinterlacer", "", "ci"));
	_filters.push_back (new Filter ("ppmd", "Median deinterlacer", "", "md"));
	_filters.push_back (new Filter ("ppfd", "FFMPEG deinterlacer", "", "fd"));
	_filters.push_back (new Filter ("ppfd", "FIR low-pass deinterlacer", "", "l5"));
	_filters.push_back (new Filter ("mcdeint", "Motion compensating deinterlacer", "mcdeint", ""));
	_filters.push_back (new Filter ("kerndeint", "Kernel deinterlacer", "kerndeint", ""));
	_filters.push_back (new Filter ("pptn", "Temporal noise reducer", "", "tn"));
	_filters.push_back (new Filter ("ppfq", "Force quantizer", "", "fq"));
	_filters.push_back (new Filter ("gradfun", "Gradient debander", "gradfun", ""));
	_filters.push_back (new Filter ("unsharp", "Unsharp mask and Gaussian blur", "unsharp", ""));
	_filters.push_back (new Filter ("denoise3d", "3D denoiser", "denoise3d", ""));
	_filters.push_back (new Filter ("hqdn3d", "High quality 3D denoiser", "hqdn3d", ""));
	_filters.push_back (new Filter ("telecine", "Telecine filter", "telecine", ""));
	_filters.push_back (new Filter ("ow", "Overcomplete wavelet denoiser", "mp=ow", ""));
}

pair<string, string>
Filter::ffmpeg_strings (vector<Filter*> const & filters)
{
	string vf;
	string pp;

	for (vector<Filter*>::const_iterator i = filters.begin(); i != filters.end(); ++i) {
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
