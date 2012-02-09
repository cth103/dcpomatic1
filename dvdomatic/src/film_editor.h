#include <gtkmm.h>

class Film;

class FilmEditor
{
public:
	FilmEditor (Film *);

	Gtk::Widget& get_widget ();

private:
	void model_to_view ();
	void view_to_model ();

	void crop_changed ();

	Gtk::Label& left_aligned_label (std::string const &) const;

	Film* _film;
	Gtk::Table _table;
	Gtk::Label _directory;
	Gtk::Entry _name;
	Gtk::ComboBoxText _format;
	Gtk::FileChooserButton _content;
	Gtk::SpinButton _left_crop;
	Gtk::SpinButton _right_crop;
	Gtk::SpinButton _top_crop;
	Gtk::SpinButton _bottom_crop;
};
