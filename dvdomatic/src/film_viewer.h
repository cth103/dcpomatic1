#include <gtkmm.h>
#include "decoder.h"

class FilmViewer
{
public:
	FilmViewer (Film *);

	Gtk::Widget& get_container () {
		return _vbox;
	}

private:
	void position_slider_changed ();
	std::string format_position_slider_value (double) const;
	void load_thumbnail (int);

	Film* _film;
	Gtk::VBox _vbox;
	Gtk::Image _image;
	Glib::RefPtr<Gdk::Pixbuf> _pixbuf;
	Gtk::HScale _position_slider;
};
