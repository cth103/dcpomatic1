/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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
 *  @brief A wx widget to edit a film's metadata, and perform various functions.
 */

#include <iostream>
#include <iomanip>
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include "lib/film.h"
#include "lib/transcode_job.h"
#include "lib/exceptions.h"
#include "lib/job_manager.h"
#include "lib/filter.h"
#include "lib/ratio.h"
#include "lib/config.h"
#include "lib/image_content.h"
#include "lib/ffmpeg_content.h"
#include "lib/sndfile_content.h"
#include "lib/dcp_content_type.h"
#include "lib/scaler.h"
#include "lib/playlist.h"
#include "lib/content.h"
#include "lib/content_factory.h"
#include "lib/dcp_content.h"
#include "lib/safe_stringstream.h"
#include "timecode.h"
#include "wx_util.h"
#include "film_editor.h"
#include "timeline_dialog.h"
#include "timing_panel.h"
#include "subtitle_panel.h"
#include "audio_panel.h"
#include "video_panel.h"
#include "content_panel.h"
#include "dcp_panel.h"

using std::string;
using std::cout;
using std::pair;
using std::fixed;
using std::setprecision;
using std::list;
using std::vector;
using std::max;
using boost::shared_ptr;
using boost::weak_ptr;
using boost::dynamic_pointer_cast;
using boost::lexical_cast;

/** @param f Film to edit */
FilmEditor::FilmEditor (shared_ptr<Film> f, wxWindow* parent)
	: wxPanel (parent)
{
	wxBoxSizer* s = new wxBoxSizer (wxVERTICAL);

	_main_notebook = new wxNotebook (this, wxID_ANY);
	s->Add (_main_notebook, 1);

	_content_panel = new ContentPanel (_main_notebook, _film);
	_main_notebook->AddPage (_content_panel->panel (), _("Content"), true);
	_dcp_panel = new DCPPanel (_main_notebook, _film);
	_main_notebook->AddPage (_dcp_panel->panel (), _("DCP"), false);
	
	set_film (f);

	JobManager::instance()->ActiveJobsChanged.connect (
		bind (&FilmEditor::active_jobs_changed, this, _1)
		);

	SetSizerAndFit (s);
}


/** Called when the metadata stored in the Film object has changed;
 *  so that we can update the GUI.
 *  @param p Property of the Film that has changed.
 */
void
FilmEditor::film_changed (Film::Property p)
{
	ensure_ui_thread ();
	
	if (!_film) {
		return;
	}

	_content_panel->film_changed (p);
	_dcp_panel->film_changed (p);
}

void
FilmEditor::film_content_changed (int property)
{
	ensure_ui_thread ();
	
	if (!_film) {
		/* We call this method ourselves (as well as using it as a signal handler)
		   so _film can be 0.
		*/
		return;
	}

	_content_panel->film_content_changed (property);
	_dcp_panel->film_content_changed (property);
}

/** Sets the Film that we are editing */
void
FilmEditor::set_film (shared_ptr<Film> f)
{
	set_general_sensitivity (f != 0);

	if (_film == f) {
		return;
	}
	
	_film = f;

	_content_panel->set_film (_film);
	_dcp_panel->set_film (_film);

	if (_film) {
		_film->Changed.connect (bind (&FilmEditor::film_changed, this, _1));
		_film->ContentChanged.connect (bind (&FilmEditor::film_content_changed, this, _2));
	}

	if (_film) {
		FileChanged (_film->directory ());
	} else {
		FileChanged ("");
	}

	if (!_film->content().empty ()) {
		_content_panel->set_selection (_film->content().front ());
	}
}

void
FilmEditor::set_general_sensitivity (bool s)
{
	_content_panel->set_general_sensitivity (s);
	_dcp_panel->set_general_sensitivity (s);
}

void
FilmEditor::active_jobs_changed (bool a)
{
	set_general_sensitivity (!a);
}
