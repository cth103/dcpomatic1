#include <iostream>
#include "film_viewer.h"
#include "film.h"

using namespace std;

FilmViewer::FilmViewer (Film* f)
	: _film (f)
{
	_vbox.pack_start (_image, true, true);
	_vbox.pack_start (_position_slider, false, false);
	_vbox.set_border_width (12);

	_position_slider.set_digits (0);
	_position_slider.signal_format_value().connect (sigc::mem_fun (*this, &FilmViewer::format_position_slider_value));
	_position_slider.signal_value_changed().connect (sigc::mem_fun (*this, &FilmViewer::position_slider_changed));

	thumbs_changed ();

	_film->ThumbsChanged.connect (sigc::mem_fun (*this, &FilmViewer::thumbs_changed));
	_film->CropChanged.connect (sigc::mem_fun (*this, &FilmViewer::crop_changed));
}

void
FilmViewer::load_thumbnail (int n)
{
	if (_film->num_thumbs() <= n) {
		return;
	}

	int const left = _film->left_crop ();
	int const right = _film->right_crop ();
	int const top = _film->top_crop ();
	int const bottom = _film->bottom_crop ();

	_pixbuf = Gdk::Pixbuf::create_from_file (_film->thumb_file (n));
	_cropped_pixbuf = Gdk::Pixbuf::create_subpixbuf (_pixbuf, left, top, _film->width() - left - right, _film->height() - top - bottom);
	_image.set (_cropped_pixbuf);
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

	if (int (v) < _film->num_thumbs ()) {
		s << _film->thumb_frame (int (v));
	} else {
		s << "-";
	}
	
	return s.str ();
}

void
FilmViewer::thumbs_changed ()
{
	if (_film->num_thumbs() > 0) {
		_position_slider.set_range (0, _film->num_thumbs () - 1);
	} else {
		_position_slider.set_range (0, 1);
	}
	
	_position_slider.set_value (0);
	reload_current_thumbnail ();
}

void
FilmViewer::crop_changed ()
{
	reload_current_thumbnail ();
}
