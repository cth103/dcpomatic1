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

#include <gtkmm.h>
#include "film.h"

class FilmEditor
{
public:
	FilmEditor (Film *);

	Gtk::Widget& get_widget ();

	void set_film (Film *);

private:
	/* Handle changes to the view */
	void name_changed ();
	void left_crop_changed ();
	void right_crop_changed ();
	void top_crop_changed ();
	void bottom_crop_changed ();
	void content_changed ();
	void format_changed ();
	void dcp_long_name_changed ();
	void dcp_pretty_name_changed ();
	void dcp_content_type_changed ();
	void dcp_frames_changed ();
	void dcp_ab_toggled ();

	/* Handle changes to the model */
	void film_changed (Film::Property);

	/* Button clicks */
	void edit_filters_clicked ();
	void make_dcp_clicked ();

	void set_things_sensitive (bool);

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
	Gtk::Label _filters;
	Gtk::Button _filters_button;
	Gtk::Entry _dcp_long_name;
	Gtk::Entry _dcp_pretty_name;
	Gtk::ComboBoxText _dcp_content_type;
	Gtk::Label _original_size;
	Gtk::Label _frames_per_second;
	Gtk::Label _audio_channels;
	Gtk::Label _audio_sample_rate;

	Gtk::Button _make_dcp_button;
	Gtk::RadioButton _dcp_whole;
	Gtk::RadioButton _dcp_for;
	Gtk::SpinButton _dcp_for_frames;
	Gtk::CheckButton _dcp_ab;
};
