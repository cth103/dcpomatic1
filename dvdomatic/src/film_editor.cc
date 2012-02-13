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
#include <boost/thread.hpp>
#include "film_editor.h"
#include "format.h"
#include "film.h"
#include "transcode_job.h"
#include "thumbs_job.h"
#include "make_mxf_job.h"
#include "make_dcp_job.h"
#include "job_manager.h"
#include "util.h"
#include "filter_dialog.h"
#include "filter.h"

using namespace std;
using namespace Gtk;

FilmEditor::FilmEditor (Film* f)
	: _film (f)
	, _filters_button ("Edit...")
	, _update_thumbs_button ("Update Thumbs")
	, _save_metadata_button ("Save Metadata")
	, _make_dcp_button ("Make DCP")
{
	_vbox.set_border_width (12);
	_vbox.set_spacing (12);
	
	Table* t = manage (new Table);
	
	t->set_row_spacings (4);
	t->set_col_spacings (12);

	/* Set up our widgets and connect to them to find out when they change */

	_directory.set_alignment (0, 0.5);

	vector<Format*> fmt = Format::get_all ();
	for (vector<Format*>::iterator i = fmt.begin(); i != fmt.end(); ++i) {
		_format.append_text ((*i)->name ());
	}

	_name.signal_changed().connect (sigc::mem_fun (*this, &FilmEditor::name_changed));

	_format.signal_changed().connect (sigc::mem_fun (*this, &FilmEditor::format_changed));
	_content.signal_file_set().connect (sigc::mem_fun (*this, &FilmEditor::content_changed));

	_left_crop.set_range (0, 1024);
	_left_crop.set_increments (1, 16);
	_left_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::left_crop_changed));

	_right_crop.set_range (0, 1024);
	_right_crop.set_increments (1, 16);
	_right_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::right_crop_changed));

	_top_crop.set_range (0, 1024);
	_top_crop.set_increments (1, 16);
	_top_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::top_crop_changed));

	_bottom_crop.set_range (0, 1024);
	_bottom_crop.set_increments (1, 16);
	_bottom_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::bottom_crop_changed));

	_filters.set_alignment (0, 0.5);
	_filters_button.signal_clicked().connect (sigc::mem_fun (*this, &FilmEditor::edit_filters_clicked));

	vector<ContentType*> const ct = ContentType::get_all ();
	for (vector<ContentType*>::const_iterator i = ct.begin(); i != ct.end(); ++i) {
		_dcp_content_type.append_text ((*i)->pretty_name ());
	}

	_dcp_long_name.signal_changed().connect (sigc::mem_fun (*this, &FilmEditor::dcp_long_name_changed));
	_dcp_pretty_name.signal_changed().connect (sigc::mem_fun (*this, &FilmEditor::dcp_pretty_name_changed));
	_dcp_content_type.signal_changed().connect (sigc::mem_fun (*this, &FilmEditor::dcp_content_type_changed));

	_original_size.set_alignment (0, 0.5);
	_frames_per_second.set_alignment (0, 0.5);

	/* Set up the table */
	
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

	Label* hint = manage (new Label ("<small>e.g. THE-BLUES-BROS_FTR_F_EN-XX_EN-GB_51-EN_2K_ST_20120101_FAC_2D_OV</small>"));
	hint->set_use_markup ();
	t->attach (*hint, 0, 2, n, n + 1);
	++n;
	
	t->attach (left_aligned_label ("DCP Pretty Name"), 0, 1, n, n + 1);
	t->attach (_dcp_pretty_name, 1, 2, n, n + 1);
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
	t->attach (_filters, 1, 2, n, n + 1);
	t->attach (_filters_button, 2, 3, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Original Size"), 0, 1, n, n + 1);
	t->attach (_original_size, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Frames Per Second"), 0, 1, n, n + 1);
	t->attach (_frames_per_second, 1, 2, n, n + 1);
	++n;

	t->show_all ();
	_vbox.pack_start (*t, false, false);

	/* Set up the buttons */

	HBox* h = manage (new HBox);
	h->set_spacing (12);
	h->pack_start (_update_thumbs_button, false, false);
	h->pack_start (_save_metadata_button, false, false);
	h->pack_start (_make_dcp_button, false, false);
	_update_thumbs_button.signal_clicked().connect (sigc::mem_fun (*this, &FilmEditor::update_thumbs_clicked));
	_save_metadata_button.signal_clicked().connect (sigc::mem_fun (*this, &FilmEditor::save_metadata_clicked));
	_make_dcp_button.signal_clicked().connect (sigc::mem_fun (*this, &FilmEditor::make_dcp_clicked));
	_vbox.pack_start (*h, false, false);

	set_film (_film);
}

Widget&
FilmEditor::get_widget ()
{
	return _vbox;
}

void
FilmEditor::left_crop_changed ()
{
	if (_film) {
		_film->set_left_crop (_left_crop.get_value ());
	}
}

void
FilmEditor::right_crop_changed ()
{
	if (_film) {
		_film->set_right_crop (_right_crop.get_value ());
	}
}

void
FilmEditor::top_crop_changed ()
{
	if (_film) {
		_film->set_top_crop (_top_crop.get_value ());
	}
}

void
FilmEditor::bottom_crop_changed ()
{
	if (_film) {
		_film->set_bottom_crop (_bottom_crop.get_value ());
	}
}

void
FilmEditor::update_thumbs_clicked ()
{
	if (!_film) {
		return;
	}
			
	ThumbsJob* j = new ThumbsJob (_film);
	j->Finished.connect (sigc::mem_fun (_film, &Film::update_thumbs_gui));
	JobManager::instance()->add (j);
}

void
FilmEditor::save_metadata_clicked ()
{
	if (_film) {
		_film->write_metadata ();
	}
}

void
FilmEditor::content_changed ()
{
	try {
		if (_film) {
			_film->set_content (_content.get_filename ());
		}
	} catch (runtime_error& e) {
		_content.set_filename (_film->directory ());
		stringstream m;
		m << "Could not set content: " << e.what() << ".";
		Gtk::MessageDialog d (m.str(), false, MESSAGE_ERROR);
		d.set_title ("DVD-o-matic");
		d.run ();
	}
}

void
FilmEditor::name_changed ()
{
	if (_film) {
		_film->set_name (_name.get_text ());
	}
}

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
		pair<string, string> p = Filter::ffmpeg_strings (_film->get_filters ());
		_filters.set_text (p.first + " " + p.second);
		break;
	}
	case Film::Name:
		_name.set_text (_film->name ());
		break;
	case Film::FramesPerSecond:
		s << _film->frames_per_second ();
		_frames_per_second.set_text (s.str ());
		break;
	case Film::Size:
		s << _film->width() << " x " << _film->height();
		_original_size.set_text (s.str ());
		break;
	case Film::DCPLongName:
		_dcp_long_name.set_text (_film->dcp_long_name ());
		break;
	case Film::DCPPrettyName:
		_dcp_pretty_name.set_text (_film->dcp_pretty_name ());
		break;
	case Film::ContentTypeChange:
		_dcp_content_type.set_active (ContentType::get_as_index (_film->dcp_content_type ()));
		break;
	}
}

void
FilmEditor::format_changed ()
{
	if (_film) {
		_film->set_format (Format::get_from_index (_format.get_active_row_number ()));
	}
}

void
FilmEditor::make_dcp_clicked ()
{
	if (!_film) {
		return;
	}
	
	JobManager::instance()->add (new TranscodeJob (_film));
	JobManager::instance()->add (new MakeMXFJob (_film, MakeMXFJob::VIDEO));
	JobManager::instance()->add (new MakeMXFJob (_film, MakeMXFJob::AUDIO));
	JobManager::instance()->add (new MakeDCPJob (_film));
}

void
FilmEditor::dcp_content_type_changed ()
{
	if (_film) {
		_film->set_dcp_content_type (ContentType::get_from_index (_dcp_content_type.get_active_row_number ()));
	}
}

void
FilmEditor::dcp_long_name_changed ()
{
	if (_film) {
		_film->set_dcp_long_name (_dcp_long_name.get_text ());
	}
}

void
FilmEditor::dcp_pretty_name_changed ()
{
	if (_film) {
		_film->set_dcp_pretty_name (_dcp_pretty_name.get_text ());
	}
}

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
	film_changed (Film::LeftCrop);
	film_changed (Film::FilmFormat);
	film_changed (Film::RightCrop);
	film_changed (Film::TopCrop);
	film_changed (Film::BottomCrop);
	film_changed (Film::Size);
	film_changed (Film::Content);
	film_changed (Film::FramesPerSecond);
	film_changed (Film::DCPLongName);
	film_changed (Film::DCPPrettyName);
	film_changed (Film::ContentTypeChange);
}

void
FilmEditor::set_things_sensitive (bool s)
{
	_name.set_sensitive (s);
	_format.set_sensitive (s);
	_content.set_sensitive (s);
	_left_crop.set_sensitive (s);
	_right_crop.set_sensitive (s);
	_top_crop.set_sensitive (s);
	_bottom_crop.set_sensitive (s);
	_dcp_long_name.set_sensitive (s);
	_dcp_pretty_name.set_sensitive (s);
	_dcp_content_type.set_sensitive (s);
	
	_update_thumbs_button.set_sensitive (s);
	_save_metadata_button.set_sensitive (s);
	_make_dcp_button.set_sensitive (s);
}

void
FilmEditor::edit_filters_clicked ()
{
	FilterDialog d (_film);
	d.run ();
}
