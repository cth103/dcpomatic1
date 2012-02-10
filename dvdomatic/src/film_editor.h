#include <gtkmm.h>
#include "film.h"

class FilmEditor
{
public:
	FilmEditor (Film *);

	Gtk::Widget& get_widget ();

private:
	/* Handle changes to the view */
	void name_changed ();
	void left_crop_changed ();
	void right_crop_changed ();
	void top_crop_changed ();
	void bottom_crop_changed ();
	void content_changed ();
	void format_changed ();

	/* Handle changes to the model */
	void film_changed (Film::Property);

	/* Button clicks */
	void update_thumbs_clicked ();
	void save_metadata_clicked ();
	void make_dcp_clicked ();

	Gtk::Label& left_aligned_label (std::string const &) const;

	Film* _film;
	Gtk::VBox _vbox;
	Gtk::Label _directory;
	Gtk::Entry _name;
	Gtk::ComboBoxText _format;
	Gtk::FileChooserButton _content;
	Gtk::SpinButton _left_crop;
	Gtk::SpinButton _right_crop;
	Gtk::SpinButton _top_crop;
	Gtk::SpinButton _bottom_crop;
	Gtk::Label _original_size;
	Gtk::Label _frames_per_second;

	Gtk::Button _update_thumbs_button;
	Gtk::Button _save_metadata_button;
	Gtk::Button _make_dcp_button;
};
