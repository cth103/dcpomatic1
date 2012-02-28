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

/** @file src/scaler.cc
 *  @brief A class to describe one of FFmpeg's software scalers.
 */

#include <iostream>
#include <cassert>
extern "C" {
#include <libswscale/swscale.h>
}
#include "scaler.h"

using namespace std;

vector<Scaler const *> Scaler::_scalers;

/** @param f FFmpeg id.
 *  @param m mplayer command line id.
 *  @param i Our id.
 *  @param n User-visible name.
 */
Scaler::Scaler (int f, int m, string i, string n)
	: _ffmpeg_id (f)
	, _mplayer_id (m)
	, _id (i)
	, _name (n)
{

}

/** @return All available scalers */
vector<Scaler const *>
Scaler::get_all ()
{
	return _scalers;
}

/** Set up the static _scalers vector; must be called before get_from_*
 *  methods are used.
 */
void
Scaler::setup_scalers ()
{
	_scalers.push_back (new Scaler (SWS_BICUBIC, 2, "bicubic", "Bicubic"));
	_scalers.push_back (new Scaler (SWS_X, 3, "x", "X"));
	_scalers.push_back (new Scaler (SWS_AREA, 5, "area", "Area"));
	_scalers.push_back (new Scaler (SWS_GAUSS, 7, "gauss", "Gaussian"));
	_scalers.push_back (new Scaler (SWS_LANCZOS, 9, "lanczos", "Lanczos"));
	_scalers.push_back (new Scaler (SWS_SINC, 8, "sinc", "Sinc"));
	_scalers.push_back (new Scaler (SWS_SPLINE, 10, "spline", "Spline"));
	_scalers.push_back (new Scaler (SWS_BILINEAR, 1, "bilinear", "Bilinear"));
	_scalers.push_back (new Scaler (SWS_FAST_BILINEAR, 0, "fastbilinear", "Fast Bilinear"));
}

/** @param id One of our ids.
 *  @return Corresponding scaler, or 0.
 */
Scaler const *
Scaler::get_from_id (string id)
{
	vector<Scaler const *>::iterator i = _scalers.begin ();
	while (i != _scalers.end() && (*i)->id() != id) {
		++i;
	}

	if (i == _scalers.end ()) {
		return 0;
	}

	return *i;
}

/** @param s A scaler from our static list.
 *  @return Index of the scaler with the list, or -1.
 */
int
Scaler::get_as_index (Scaler const * s)
{
	vector<Scaler*>::size_type i = 0;
	while (i < _scalers.size() && _scalers[i] != s) {
		++i;
	}

	if (i == _scalers.size ()) {
		return -1;
	}

	return i;
}

/** @param i An index returned from get_as_index().
 *  @return Corresponding scaler.
 */
Scaler const *
Scaler::get_from_index (int i)
{
	assert (i <= int(_scalers.size ()));
	return _scalers[i];
}
