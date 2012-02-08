#include <iostream>
#include "film_viewer.h"

using namespace std;

FilmViewer::FilmViewer (Film* f)
	: Decoder (f, 1224, 512)
{
	decode_audio (false);
	
	_vbox.pack_start (_image, true, true);
	_vbox.pack_start (_position_slider, false, false);

	_pixbuf = Gdk::Pixbuf::create (Gdk::COLORSPACE_RGB, false, 8, 1224, 512);
	_image.set (_pixbuf);

	_position_slider.set_range (0, 1e9);
	_position_slider.set_value (0);
	_position_slider.signal_value_changed().connect (sigc::mem_fun (*this, &FilmViewer::position_slider_changed));

	position_slider_changed ();
}

void
FilmViewer::process_video (uint8_t* data, int line_size)
{
	guint8* p = _pixbuf->get_pixels ();
	memcpy (p, data, line_size * 512);
	_image.queue_draw ();
}


void
FilmViewer::position_slider_changed ()
{
	while (1) {
		PassResult const r = pass ();
		if (r == PASS_VIDEO || r == PASS_DONE) {
			break;
		}
	}
}
