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

#include "film_editor_panel.h"

class wxCheckBox;
class wxSpinCtrl;

class SubtitlePanel : public FilmEditorPanel
{
public:
	SubtitlePanel (FilmEditor *);

	void film_changed (Film::Property);
	void film_content_changed (
		boost::shared_ptr<Content>,
		boost::shared_ptr<AudioContent>,
		boost::shared_ptr<SubtitleContent>,
		boost::shared_ptr<FFmpegContent>,
		int);

	void setup_control_sensitivity ();
	
private:
	void with_subtitles_toggled (wxCommandEvent &);
	void offset_changed (wxCommandEvent &);
	void scale_changed (wxCommandEvent &);
	void stream_changed (wxCommandEvent &);
	
	wxCheckBox* _with_subtitles;
	wxSpinCtrl* _offset;
	wxSpinCtrl* _scale;
	wxChoice* _stream;
};
