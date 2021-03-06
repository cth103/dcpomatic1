/*
    Copyright (C) 2012-2013 Carl Hetherington <cth@carlh.net>

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

#ifndef DCPOMATIC_FILM_EDITOR_PANEL_H
#define DCPOMATIC_FILM_EDITOR_PANEL_H

#include <boost/shared_ptr.hpp>
#include <wx/wx.h>
#include "lib/film.h"

class FilmEditor;
class Content;

class FilmEditorPanel : public wxPanel
{
public:
	FilmEditorPanel (FilmEditor *, wxString);

	virtual void film_changed (Film::Property) {}
	/** Called when a given property of one of the selected Contents changes */
	virtual void film_content_changed (int) = 0;
	/** Called when the list of selected Contents changes */
	virtual void content_selection_changed () = 0;

protected:
	FilmEditor* _editor;
	wxSizer* _sizer;
};

#endif
