#include <gtkmm.h>
#include "decoder.h"

class FilmViewer
{
public:
	FilmViewer (Film *);

	Gtk::Widget& get_widget () {
		return _vbox;
	}

private:
	void position_slider_changed ();
	std::string format_position_slider_value (double) const;
	void load_thumbnail (int);
	void thumbs_changed ();
	void crop_changed ();
	void reload_current_thumbnail ();

	Film* _film;
	Gtk::VBox _vbox;
	Gtk::Image _image;
	Glib::RefPtr<Gdk::Pixbuf> _pixbuf;
	Glib::RefPtr<Gdk::Pixbuf> _cropped_pixbuf;
	Gtk::HScale _position_slider;
};
