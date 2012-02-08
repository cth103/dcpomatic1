#include <gtkmm.h>
#include "decoder.h"

class FilmViewer : public Decoder
{
public:
	FilmViewer (Film *);

	Gtk::Widget& get_container () {
		return _vbox;
	}

	void process_video (uint8_t *, int);
	void process_audio (uint8_t *, int, int) {}
	
private:
	void position_slider_changed ();

	Gtk::VBox _vbox;
	Gtk::Image _image;
	Glib::RefPtr<Gdk::Pixbuf> _pixbuf;
	Gtk::HScale _position_slider;
};
