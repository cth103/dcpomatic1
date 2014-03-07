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

#include "film_editor_panel.h"

class wxCheckBox;
class wxSpinCtrl;
class SubtitleView;

class SubtitlePanel : public FilmEditorPanel
{
public:
	SubtitlePanel (FilmEditor *);

	void film_changed (Film::Property);
	void film_content_changed (int);
	void content_selection_changed ();
	
private:
	void with_subtitles_toggled ();
	void x_offset_changed ();
	void y_offset_changed ();
	void scale_changed ();
	void stream_changed ();
	void view_clicked ();

	void setup_sensitivity ();
	
	wxCheckBox* _with_subtitles;
	wxSpinCtrl* _x_offset;
	wxSpinCtrl* _y_offset;
	wxSpinCtrl* _scale;
	wxChoice* _stream;
	wxButton* _view_button;
	SubtitleView* _view;
};
