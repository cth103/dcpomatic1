#include <iostream>
extern "C" {
#include <tiffio.h>
}
#include "film_viewer.h"
#include "film.h"

using namespace std;

FilmViewer::FilmViewer (Film* f)
	: _film (f)
{
	_vbox.pack_start (_image, true, true);
	_vbox.pack_start (_position_slider, false, false);

	_pixbuf = Gdk::Pixbuf::create (Gdk::COLORSPACE_RGB, false, 8, _film->thumb_width(), _film->thumb_height());
	_image.set (_pixbuf);

	_position_slider.set_range (0, _film->num_thumbs () - 1);
	_position_slider.set_value (0);
	_position_slider.set_digits (0);
	_position_slider.signal_format_value().connect (sigc::mem_fun (*this, &FilmViewer::format_position_slider_value));
	_position_slider.signal_value_changed().connect (sigc::mem_fun (*this, &FilmViewer::position_slider_changed));

	position_slider_changed ();
}

void
FilmViewer::load_thumbnail (int n)
{
	TIFF* t = TIFFOpen (_film->thumb_file(n).c_str (), "r");
	if (t == 0) {
		throw runtime_error ("Could not open thumbnail file");
	}

	int const w = _film->thumb_width ();
	int const h = _film->thumb_height ();
	
	uint8_t* tiff_buffer = new uint8_t[w * h * 3];
	uint8_t* p = tiff_buffer;
	for (tstrip_t i = 0; i < TIFFNumberOfStrips (t); ++i) {
		p += TIFFReadEncodedStrip (t, i, p, -1);
	}

	p = tiff_buffer;
	uint8_t* q = _pixbuf->get_pixels ();
	for (int i = 0; i < h; ++i) {
		memcpy (q, p, w * 3);
		q += _pixbuf->get_rowstride ();
		p += w * 3;
	}

	_image.queue_draw ();
}


void
FilmViewer::position_slider_changed ()
{
	load_thumbnail (_position_slider.get_value ());
}

string
FilmViewer::format_position_slider_value (double v) const
{
	stringstream s;
	s << _film->thumb_frame (int (v));
	return s.str ();
}
