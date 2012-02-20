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

#include <iostream>
#include <cassert>
extern "C" {
#include <libswscale/swscale.h>
}
#include "scaler.h"

using namespace std;

vector<Scaler const *> Scaler::_scalers;

Scaler::Scaler (int f, string i, string n)
	: _ffmpeg_id (f)
	, _id (i)
	, _name (n)
{

}

vector<Scaler const *>
Scaler::get_all ()
{
	return _scalers;
}

void
Scaler::setup_scalers ()
{
	cout << "setup.\n";
	_scalers.push_back (new Scaler (SWS_BICUBIC, "bicubic", "Bicubic"));
	_scalers.push_back (new Scaler (SWS_X, "x", "X"));
	_scalers.push_back (new Scaler (SWS_AREA, "area", "Area"));
	_scalers.push_back (new Scaler (SWS_GAUSS, "gauss", "Gaussian"));
	_scalers.push_back (new Scaler (SWS_LANCZOS, "lanczos", "Lanczos"));
	_scalers.push_back (new Scaler (SWS_SINC, "sinc", "Sinc"));
	_scalers.push_back (new Scaler (SWS_SPLINE, "spline", "Spline"));
	_scalers.push_back (new Scaler (SWS_BILINEAR, "bilinear", "Bilinear"));
	_scalers.push_back (new Scaler (SWS_FAST_BILINEAR, "fastbilinear", "Fast Bilinear"));
}

Scaler const *
Scaler::get_from_id (string id)
{
	cout << "scaler from " << id << "\n";
	vector<Scaler const *>::iterator i = _scalers.begin ();
	while (i != _scalers.end() && (*i)->id() != id) {
		++i;
	}

	if (i == _scalers.end ()) {
		cout << "0\n";
		return 0;
	}

	cout << *i << "\n";
	return *i;
}

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

Scaler const *
Scaler::get_from_index (int i)
{
	assert (i <= int(_scalers.size ()));
	return _scalers[i];
}
