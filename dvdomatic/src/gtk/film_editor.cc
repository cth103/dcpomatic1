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

/** @file src/film_editor.cc
 *  @brief A GTK widget to edit a film's metadata, and perform various functions.
 */

#include <iostream>
#include <gtkmm.h>
#include <boost/thread.hpp>
#include "lib/format.h"
#include "lib/film.h"
#include "lib/transcode_job.h"
#include "lib/ab_transcode_job.h"
#include "lib/thumbs_job.h"
#include "lib/make_mxf_job.h"
#include "lib/make_dcp_job.h"
#include "lib/job_manager.h"
#include "lib/filter.h"
#include "lib/copy_from_dvd_job.h"
#include "filter_dialog.h"
#include "gtk_util.h"
#include "film_editor.h"

using namespace std;
using namespace boost;
using namespace Gtk;

/** @param f Film to edit */
FilmEditor::FilmEditor (Film* f)
	: _film (f)
	, _copy_from_dvd_button ("Copy from DVD")
	, _filters_button ("Edit...")
	, _guess_dcp_long_name ("Guess")
	, _examine_content_button ("Examine Content")
	, _make_dcp_button ("Make DCP")
	, _dcp_whole ("Whole Film")
	, _dcp_for ("For")
	, _dcp_ab ("A/B")
{
	_vbox.set_border_width (12);
	_vbox.set_spacing (12);
	
	/* Set up our editing widgets */
	_directory.set_alignment (0, 0.5);
	_left_crop.set_range (0, 1024);
	_left_crop.set_increments (1, 16);
	_top_crop.set_range (0, 1024);
	_top_crop.set_increments (1, 16);
	_right_crop.set_range (0, 1024);
	_right_crop.set_increments (1, 16);
	_bottom_crop.set_range (0, 1024);
	_bottom_crop.set_increments (1, 16);
	_filters.set_alignment (0, 0.5);

	vector<Format const *> fmt = Format::get_all ();
	for (vector<Format const *>::iterator i = fmt.begin(); i != fmt.end(); ++i) {
		_format.append_text ((*i)->name ());
	}

	_frames_per_second.set_increments (1, 5);
	_frames_per_second.set_digits (2);
	_frames_per_second.set_range (0, 60);

	vector<ContentType const *> const ct = ContentType::get_all ();
	for (vector<ContentType const *>::const_iterator i = ct.begin(); i != ct.end(); ++i) {
		_dcp_content_type.append_text ((*i)->pretty_name ());
	}

	vector<Scaler const *> const sc = Scaler::get_all ();
	for (vector<Scaler const *>::const_iterator i = sc.begin(); i != sc.end(); ++i) {
		_scaler.append_text ((*i)->name ());
	}
	
	_original_size.set_alignment (0, 0.5);
	_length.set_alignment (0, 0.5);
	_audio_channels.set_alignment (0, 0.5);
	_audio_sample_rate.set_alignment (0, 0.5);

	Gtk::RadioButtonGroup g = _dcp_whole.get_group ();
	_dcp_for.set_group (g);
	_dcp_for_frames.set_range (24, 65536);
	_dcp_for_frames.set_increments (24, 60 * 24);

	/* And set their values from the Film */
	set_film (f);
	
	/* Now connect to them, since initial values are safely set */
	_name.signal_changed().connect (sigc::mem_fun (*this, &FilmEditor::name_changed));
	_frames_per_second.signal_changed().connect (sigc::mem_fun (*this, &FilmEditor::frames_per_second_changed));
	_format.signal_changed().connect (sigc::mem_fun (*this, &FilmEditor::format_changed));
	_content.signal_file_set().connect (sigc::mem_fun (*this, &FilmEditor::content_changed));
	_left_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::left_crop_changed));
	_right_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::right_crop_changed));
	_top_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::top_crop_changed));
	_bottom_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::bottom_crop_changed));
	_filters_button.signal_clicked().connect (sigc::mem_fun (*this, &FilmEditor::edit_filters_clicked));
	_scaler.signal_changed().connect (sigc::mem_fun (*this, &FilmEditor::scaler_changed));
	_dcp_long_name.signal_changed().connect (sigc::mem_fun (*this, &FilmEditor::dcp_long_name_changed));
	_guess_dcp_long_name.signal_toggled().connect (sigc::mem_fun (*this, &FilmEditor::guess_dcp_long_name_toggled));
	_dcp_content_type.signal_changed().connect (sigc::mem_fun (*this, &FilmEditor::dcp_content_type_changed));
	_dcp_for.signal_toggled().connect (sigc::mem_fun (*this, &FilmEditor::dcp_frames_changed));
	_dcp_for_frames.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::dcp_frames_changed));
	_dcp_ab.signal_toggled().connect (sigc::mem_fun (*this, &FilmEditor::dcp_ab_toggled));

	/* Set up the table */

	Table* t = manage (new Table);
	
	t->set_row_spacings (4);
	t->set_col_spacings (12);
	
	int n = 0;
	t->attach (left_aligned_label ("Directory"), 0, 1, n, n + 1);
	t->attach (_directory, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Name"), 0, 1, n, n + 1);
	t->attach (_name, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("DCP Long Name"), 0, 1, n, n + 1);
	t->attach (_dcp_long_name, 1, 2, n, n + 1);
	++n;
	t->attach (_guess_dcp_long_name, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Frames Per Second"), 0, 1, n, n + 1);
	t->attach (_frames_per_second, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Format"), 0, 1, n, n + 1);
	t->attach (_format, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Content"), 0, 1, n, n + 1);
	t->attach (_content, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Content Type"), 0, 1, n, n + 1);
	t->attach (_dcp_content_type, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Left Crop"), 0, 1, n, n + 1);
	t->attach (_left_crop, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Right Crop"), 0, 1, n, n + 1);
	t->attach (_right_crop, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Top Crop"), 0, 1, n, n + 1);
	t->attach (_top_crop, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Bottom Crop"), 0, 1, n, n + 1);
	t->attach (_bottom_crop, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Filters"), 0, 1, n, n + 1);
	HBox* fb = manage (new HBox);
	fb->set_spacing (4);
	fb->pack_start (_filters, true, true);
	fb->pack_start (_filters_button, false, false);
	t->attach (*fb, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Scaler"), 0, 1, n, n + 1);
	t->attach (_scaler, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Original Size"), 0, 1, n, n + 1);
	t->attach (_original_size, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Length"), 0, 1, n, n + 1);
	t->attach (_length, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Audio Channels"), 0, 1, n, n + 1);
	t->attach (_audio_channels, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Audio Sample Rate"), 0, 1, n, n + 1);
	t->attach (_audio_sample_rate, 1, 2, n, n + 1);
	++n;

	t->show_all ();
	_vbox.pack_start (*t, false, false);

	_copy_from_dvd_button.signal_clicked().connect (sigc::mem_fun (*this, &FilmEditor::copy_from_dvd_clicked));
	_examine_content_button.signal_clicked().connect (sigc::mem_fun (*this, &FilmEditor::examine_content_clicked));
	_make_dcp_button.signal_clicked().connect (sigc::mem_fun (*this, &FilmEditor::make_dcp_clicked));

	HBox* h = manage (new HBox);
	h->set_spacing (12);
	h->pack_start (_examine_content_button, false, false);
	h->pack_start (_copy_from_dvd_button, false, false);
	_vbox.pack_start (*h, false, false);
	
	h = manage (new HBox);
	h->set_spacing (12);
	h->pack_start (_make_dcp_button, false, false);
	h->pack_start (_dcp_whole, false, false);
	h->pack_start (_dcp_for, false, false);
	h->pack_start (_dcp_for_frames, true, true);
	h->pack_start (left_aligned_label ("frames"), false, false);
	h->pack_start (_dcp_ab);
	_vbox.pack_start (*h, false, false);
	
}

/** @return Our main widget, which contains everything else */
Widget&
FilmEditor::get_widget ()
{
	return _vbox;
}

/** Called when the left crop widget has been changed */
void
FilmEditor::left_crop_changed ()
{
	if (_film) {
		_film->set_left_crop (_left_crop.get_value ());
	}
}

/** Called when the right crop widget has been changed */
void
FilmEditor::right_crop_changed ()
{
	if (_film) {
		_film->set_right_crop (_right_crop.get_value ());
	}
}

/** Called when the top crop widget has been changed */
void
FilmEditor::top_crop_changed ()
{
	if (_film) {
		_film->set_top_crop (_top_crop.get_value ());
	}
}

/** Called when the bottom crop value has been changed */
void
FilmEditor::bottom_crop_changed ()
{
	if (_film) {
		_film->set_bottom_crop (_bottom_crop.get_value ());
	}
}

/** Called when the content filename has been changed */
void
FilmEditor::content_changed ()
{
	if (!_film) {
		return;
	}
	try {
		/* XXX: hack; set to parent directory if we get a TIFF */
		string const f = _content.get_filename ();
#if BOOST_FILESYSTEM_VERSION == 3		
		string const ext = filesystem::path(f).extension().string();
#else
		string const ext = filesystem::path(f).extension();
#endif		
		if (ext == ".tif" || ext == ".tiff") {
			_film->set_content (filesystem::path (f).branch_path().string ());
		} else {
			_film->set_content (f);
		}
	} catch (std::exception& e) {
		_content.set_filename (_film->directory ());
		stringstream m;
		m << "Could not set content: " << e.what() << ".";
		Gtk::MessageDialog d (m.str(), false, MESSAGE_ERROR);
		d.set_title ("DVD-o-matic");
		d.run ();
	}
}

/** Called when the number of DCP frames has been changed */
void
FilmEditor::dcp_frames_changed ()
{
	if (!_film) {
		return;
	}

	if (_dcp_for.get_active ()) {
		_film->set_dcp_frames (_dcp_for_frames.get_value ());
	} else {
		_film->set_dcp_frames (0);
	}
}

/** Called when the DCP A/B switch has been toggled */
void
FilmEditor::dcp_ab_toggled ()
{
	if (_film) {
		_film->set_dcp_ab (_dcp_ab.get_active ());
	}
}

/** Called when the name widget has been changed */
void
FilmEditor::name_changed ()
{
	if (_film) {
		_film->set_name (_name.get_text ());
	}
}

/** Called when the metadata stored in the Film object has changed;
 *  so that we can update the GUI.
 *  @param p Property of the Film that has changed.
 */
void
FilmEditor::film_changed (Film::Property p)
{
	if (!_film) {
		return;
	}
	
	stringstream s;
		
	switch (p) {
	case Film::Content:
		_content.set_filename (_film->content ());
		break;
	case Film::FilmFormat:
		_format.set_active (Format::get_as_index (_film->format ()));
		break;
	case Film::LeftCrop:
		_left_crop.set_value (_film->left_crop ());
		break;
	case Film::RightCrop:
		_right_crop.set_value (_film->right_crop ());
		break;
	case Film::TopCrop:
		_top_crop.set_value (_film->top_crop ());
		break;
	case Film::BottomCrop:
		_bottom_crop.set_value (_film->bottom_crop ());
		break;
	case Film::Filters:
	{
		pair<string, string> p = Filter::ffmpeg_strings (_film->filters ());
		_filters.set_text (p.first + " " + p.second);
		break;
	}
	case Film::Name:
		_name.set_text (_film->name ());
		break;
	case Film::FramesPerSecond:
		_frames_per_second.set_value (_film->frames_per_second ());
		break;
	case Film::AudioChannels:
		s << _film->audio_channels ();
		_audio_channels.set_text (s.str ());
		break;
	case Film::AudioSampleRate:
		s << _film->audio_sample_rate ();
		_audio_sample_rate.set_text (s.str ());
		break;
	case Film::FilmSize:
		s << _film->size().width << " x " << _film->size().height;
		_original_size.set_text (s.str ());
		break;
	case Film::Length:
		if (_film->frames_per_second() > 0) {
			s << _film->length() << " frames; " << seconds_to_hms (_film->length() / _film->frames_per_second());
		} else {
			s << _film->length() << " frames";
		}
		_length.set_text (s.str ());
		break;
	case Film::DCPLongName:
		_dcp_long_name.set_text (_film->dcp_long_name ());
		break;
	case Film::GuessDCPLongName:
		_guess_dcp_long_name.set_active (_film->guess_dcp_long_name ());
		break;
	case Film::DCPContentType:
		_dcp_content_type.set_active (ContentType::get_as_index (_film->dcp_content_type ()));
		break;
	case Film::Thumbs:
		break;
	case Film::DCPFrames:
		if (_film->dcp_frames() == 0) {
			_dcp_whole.set_active (true);
		} else {
			_dcp_for.set_active (true);
			_dcp_for_frames.set_value (_film->dcp_frames ());
		}
		break;
	case Film::DCPAB:
		_dcp_ab.set_active (_film->dcp_ab ());
		break;
	case Film::FilmScaler:
		_scaler.set_active (Scaler::get_as_index (_film->scaler ()));
		break;
	}
}

/** Called when the format widget has been changed */
void
FilmEditor::format_changed ()
{
	if (_film) {
		_film->set_format (Format::get_from_index (_format.get_active_row_number ()));
	}
}

/** Called when the `Make DCP' button has been clicked */
void
FilmEditor::make_dcp_clicked ()
{
	if (!_film) {
		return;
	}

	try {
		_film->make_dcp ();
	} catch (std::exception& e) {
		stringstream s;
		s << "Could not make DCP: " << e.what () << ".";
		error_dialog (s.str ());
	}
}

/** Called when the DCP content type widget has been changed */
void
FilmEditor::dcp_content_type_changed ()
{
	if (_film) {
		_film->set_dcp_content_type (ContentType::get_from_index (_dcp_content_type.get_active_row_number ()));
	}
}

/** Called when the DCP long name widget has been changed */
void
FilmEditor::dcp_long_name_changed ()
{
	if (_film) {
		_film->set_dcp_long_name (_dcp_long_name.get_text ());
	}
}

/** Sets the Film that we are editing */
void
FilmEditor::set_film (Film* f)
{
	_film = f;

	set_things_sensitive (_film != 0);

	if (_film) {
		_film->Changed.connect (sigc::mem_fun (*this, &FilmEditor::film_changed));
	}

	if (_film) {
		_directory.set_text (_film->directory ());
	} else {
		_directory.set_text ("");
	}
	
	film_changed (Film::Name);
	film_changed (Film::Content);
	film_changed (Film::DCPLongName);
	film_changed (Film::GuessDCPLongName);
	film_changed (Film::DCPContentType);
	film_changed (Film::FilmFormat);
	film_changed (Film::LeftCrop);
	film_changed (Film::RightCrop);
	film_changed (Film::TopCrop);
	film_changed (Film::BottomCrop);
	film_changed (Film::Filters);
	film_changed (Film::DCPFrames);
	film_changed (Film::DCPAB);
	film_changed (Film::FilmSize);
	film_changed (Film::Length);
	film_changed (Film::FramesPerSecond);
	film_changed (Film::AudioChannels);
	film_changed (Film::AudioSampleRate);
	film_changed (Film::FilmScaler);
}

/** Updates the sensitivity of lots of widgets to a given value.
 *  @param s true to make sensitive, false to make insensitive.
 */
void
FilmEditor::set_things_sensitive (bool s)
{
	_name.set_sensitive (s);
	_format.set_sensitive (s);
	_content.set_sensitive (s);
	_copy_from_dvd_button.set_sensitive (s);
	_left_crop.set_sensitive (s);
	_right_crop.set_sensitive (s);
	_top_crop.set_sensitive (s);
	_bottom_crop.set_sensitive (s);
	_filters_button.set_sensitive (s);
	_scaler.set_sensitive (s);
	_dcp_long_name.set_sensitive (s);
	_guess_dcp_long_name.set_sensitive (s);
	_dcp_content_type.set_sensitive (s);
	_make_dcp_button.set_sensitive (s);
	_dcp_whole.set_sensitive (s);
	_dcp_for.set_sensitive (s);
	_dcp_for_frames.set_sensitive (s);
	_dcp_ab.set_sensitive (s);
}

/** Called when the `Edit filters' button has been clicked */
void
FilmEditor::edit_filters_clicked ()
{
	FilterDialog d (_film);
	d.run ();
}

/** Called when the selector to guess the DCP long name has been toggled */
void
FilmEditor::guess_dcp_long_name_toggled ()
{
	if (!_film) {
		return;
	}

	_film->set_guess_dcp_long_name (_guess_dcp_long_name.get_active ());
}

/** Called when the `Copy from DVD' button has been clicked */
void
FilmEditor::copy_from_dvd_clicked ()
{
	shared_ptr<Job> j (new CopyFromDVDJob (_film->state_copy (), _film->log ()));
	j->Finished.connect (sigc::mem_fun (_film, &Film::copy_from_dvd_post_gui));
	JobManager::instance()->add (j);
}

/** Called when the `Examine content' button has been clicked */
void
FilmEditor::examine_content_clicked ()
{
	_film->examine_content ();
}

/** Called when the scaler widget has been changed */
void
FilmEditor::scaler_changed ()
{
	if (_film) {
		_film->set_scaler (Scaler::get_from_index (_scaler.get_active_row_number ()));
	}
}

/** Called when the frames per second widget has been changed */
void
FilmEditor::frames_per_second_changed ()
{
	if (_film) {
		_film->set_frames_per_second (_frames_per_second.get_value ());
	}
}
