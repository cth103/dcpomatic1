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

/** @file src/film_editor.h
 *  @brief A GTK widget to edit a film's metadata, and perform various functions.
 */

#include <gtkmm.h>

class Film;

/** @class FilmEditor
 *  @brief A GTK widget to edit a film's metadata, and perform various functions.
 */
class FilmEditor
{
public:
	FilmEditor (Film *);

	Gtk::Widget& widget ();

	void set_film (Film *);

private:
	/* Handle changes to the view */
	void name_changed ();
	void left_crop_changed ();
	void right_crop_changed ();
	void top_crop_changed ();
	void bottom_crop_changed ();
	void content_changed ();
	void frames_per_second_changed ();
	void format_changed ();
	void dcp_long_name_changed ();
	void guess_dcp_long_name_toggled ();
	void dcp_content_type_changed ();
	void dcp_frames_changed ();
	void dcp_ab_toggled ();
	void scaler_changed ();
	void audio_gain_changed ();

	/* Handle changes to the model */
	void film_changed (Film::Property);

	/* Button clicks */
	void edit_filters_clicked ();
	void copy_from_dvd_clicked ();
	void examine_content_clicked ();
	void make_dcp_clicked ();

	void set_things_sensitive (bool);

	/** The film we are editing */
	Film* _film;
	/** The overall VBox containing our widget */
	Gtk::VBox _vbox;
	/** The Film's directory */
	Gtk::Label _directory;
	/** The Film's name */
	Gtk::Entry _name;
	/** The Film's frames per second */
	Gtk::SpinButton _frames_per_second;
	/** The Film's format */
	Gtk::ComboBoxText _format;
	/** The Film's content file */
	Gtk::FileChooserButton _content;
	/** Button to copy content from a DVD */
	Gtk::Button _copy_from_dvd_button;
	/** The Film's left crop */
	Gtk::SpinButton _left_crop;
	/** The Film's right crop */
	Gtk::SpinButton _right_crop;
	/** The Film's top crop */
	Gtk::SpinButton _top_crop;
	/** The Film's bottom crop */
	Gtk::SpinButton _bottom_crop;
	/** Currently-applied filters */
	Gtk::Label _filters;
	/** Button to open the filters dialogue */
	Gtk::Button _filters_button;
	/** The Film's scaler */
	Gtk::ComboBoxText _scaler;
	/** The Film's audio gain */
	Gtk::SpinButton _audio_gain;
	/** The Film's DCP long name */
	Gtk::Entry _dcp_long_name;
	/** Button to choose whether to guess the contents of _dcp_long_name */
	Gtk::CheckButton _guess_dcp_long_name;
	/** The Film's DCP content type */
	Gtk::ComboBoxText _dcp_content_type;
	/** The Film's original size */
	Gtk::Label _original_size;
	/** The Film's length */
	Gtk::Label _length;
	/** The Film's audio details */
	Gtk::Label _audio;

	/** Button to start an examination of the Film's content */
	Gtk::Button _examine_content_button;
	/** Button to start making a DCP */
	Gtk::Button _make_dcp_button;
	/** Selector to make a DCP of the whole Film */
	Gtk::RadioButton _dcp_whole;
	/** Selector to make a DCP for part of the Film */
	Gtk::RadioButton _dcp_for;
	/** Number of frames to make the DCP for if _dcp_for is selected */
	Gtk::SpinButton _dcp_for_frames;
	/** Selector to generate an A/B comparison DCP */
	Gtk::CheckButton _dcp_ab;
};
