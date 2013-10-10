/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

#include <list>
#include <wx/graphics.h>
#include "lib/playlist.h"
#include "film_editor.h"
#include "timeline_dialog.h"
#include "wx_util.h"

using std::list;
using std::cout;
using boost::shared_ptr;

TimelineDialog::TimelineDialog (FilmEditor* ed, shared_ptr<Film> film)
	: wxDialog (ed, wxID_ANY, _("Timeline"), wxDefaultPosition, wxSize (640, 512), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxFULL_REPAINT_ON_RESIZE)
	, _timeline (this, ed, film)
{
	wxBoxSizer* sizer = new wxBoxSizer (wxVERTICAL);

	wxBoxSizer* controls = new wxBoxSizer (wxHORIZONTAL);
	_snap = new wxCheckBox (this, wxID_ANY, _("Snap"));
	controls->Add (_snap);

	sizer->Add (controls, 0, wxALL, 12);
	sizer->Add (&_timeline, 1, wxEXPAND | wxALL, 12);

	SetSizer (sizer);
	sizer->Layout ();
	sizer->SetSizeHints (this);

	_snap->SetValue (_timeline.snap ());
	_snap->Bind (wxEVT_COMMAND_CHECKBOX_CLICKED, boost::bind (&TimelineDialog::snap_toggled, this));
}

void
TimelineDialog::snap_toggled ()
{
	_timeline.set_snap (_snap->GetValue ());
}
