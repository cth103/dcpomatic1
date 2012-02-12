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
#include <iomanip>
#include "film_viewer.h"
#include "film.h"
#include "format.h"
#include "util.h"

using namespace std;

FilmViewer::FilmViewer (Film* f)
	: _film (f)
{
	_scroller.add (_image);

	Gtk::HBox* controls = manage (new Gtk::HBox);
	controls->set_spacing (6);
	controls->pack_start (_position_slider);
	
	_vbox.pack_start (_scroller, true, true);
	_vbox.pack_start (*controls, false, false);
	_vbox.set_border_width (12);

	_position_slider.set_digits (0);
	_position_slider.signal_format_value().connect (sigc::mem_fun (*this, &FilmViewer::format_position_slider_value));
	_position_slider.signal_value_changed().connect (sigc::mem_fun (*this, &FilmViewer::position_slider_changed));

	set_film (_film);
}

void
FilmViewer::load_thumbnail (int n)
{
	if (_film == 0 || _film->num_thumbs() <= n) {
		return;
	}

	int const left = _film->left_crop ();
	int const right = _film->right_crop ();
	int const top = _film->top_crop ();
	int const bottom = _film->bottom_crop ();

	_pixbuf = Gdk::Pixbuf::create_from_file (_film->thumb_file (n));

	int const cw = _film->width() - left - right;
	int const ch = _film->height() - top - bottom;
	_cropped_pixbuf = Gdk::Pixbuf::create_subpixbuf (_pixbuf, left, top, cw, ch);
	update_scaled_pixbuf ();
	_image.set (_scaled_pixbuf);
}

void
FilmViewer::reload_current_thumbnail ()
{
	load_thumbnail (_position_slider.get_value ());
}

void
FilmViewer::position_slider_changed ()
{
	reload_current_thumbnail ();
}

string
FilmViewer::format_position_slider_value (double v) const
{
	stringstream s;

	if (_film && int (v) < _film->num_thumbs ()) {
		int const f = _film->thumb_frame (int (v));
		s << f << " " << seconds_to_hms (f / _film->frames_per_second ());
	} else {
		s << "-";
	}
	
	return s.str ();
}

void
FilmViewer::thumbs_changed ()
{
	if (_film && _film->num_thumbs() > 0) {
		_position_slider.set_range (0, _film->num_thumbs () - 1);
	} else {
		_position_slider.set_range (0, 1);
	}
	
	_position_slider.set_value (0);
	reload_current_thumbnail ();
}

void
FilmViewer::film_changed (Film::Property p)
{
	if (p == Film::LeftCrop || p == Film::RightCrop || p == Film::TopCrop || p == Film::BottomCrop) {
		reload_current_thumbnail ();
	}
}

void
FilmViewer::set_film (Film* f)
{
	_film = f;

	if (!_film) {
		return;
	}
	
	_film->Changed.connect (sigc::mem_fun (*this, &FilmViewer::film_changed));
	_film->ThumbsChanged.connect (sigc::mem_fun (*this, &FilmViewer::thumbs_changed));
		
	thumbs_changed ();
}

pair<int, int>
FilmViewer::scaled_pixbuf_size () const
{
	if (_film == 0) {
		return make_pair (0, 0);
	}
	
	int const cw = _film->width() - _film->left_crop() - _film->right_crop(); 
	int const ch = _film->height() - _film->top_crop() - _film->bottom_crop();

	float ratio = 1;
	if (_film->format()) {
		ratio = _film->format()->ratio_as_float() * _film->height() / _film->width();
	}

	Gtk::Allocation const a = _scroller.get_allocation ();
	float const zoom = min (float (a.get_width()) / (cw * ratio), float (a.get_height()) / cw);
	return make_pair (cw * zoom * ratio, ch * zoom);
}
	
void
FilmViewer::update_scaled_pixbuf ()
{
	pair<int, int> const s = scaled_pixbuf_size ();
	
	if (s.first > 0 && s.second > 0) {
		_scaled_pixbuf = _cropped_pixbuf->scale_simple (s.first, s.second, Gdk::INTERP_HYPER);
		_image.set (_scaled_pixbuf);
	}
}
