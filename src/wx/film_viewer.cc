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

/** @file  src/film_viewer.cc
 *  @brief A wx widget to view `thumbnails' of a Film.
 */

#include <iostream>
#include <iomanip>
#include "lib/film.h"
#include "lib/format.h"
#include "lib/util.h"
#include "lib/thumbs_job.h"
#include "lib/job_manager.h"
#include "lib/film_state.h"
#include "lib/options.h"
#include "lib/subtitle.h"
#include "film_viewer.h"
#include "wx_util.h"

using namespace std;
using namespace boost;

class ThumbPanel : public wxPanel
{
public:
	ThumbPanel (wxPanel* parent, Film* film)
		: wxPanel (parent)
		, _film (film)
		, _frame_rebuild_needed (false)
		, _composition_needed (false)
	{}

	/** Handle a paint event */
	void paint_event (wxPaintEvent& ev)
	{
		if (!_film || _film->num_thumbs() == 0) {
			wxPaintDC dc (this);
			return;
		}

		if (_frame_rebuild_needed) {
			_image.reset (new wxImage (std_to_wx (_film->thumb_file (_index))));

			_subtitle.reset ();
			pair<Position, string> s = _film->thumb_subtitle (_index);
			if (!s.second.empty ()) {
				_subtitle.reset (new SubtitleView (s.first, std_to_wx (s.second)));
			}

			_frame_rebuild_needed = false;
			compose ();
		}

		if (_composition_needed) {
			compose ();
		}

		wxPaintDC dc (this);
		if (_bitmap) {
			dc.DrawBitmap (*_bitmap, 0, 0, false);
		}

		if (_film->with_subtitles() && _subtitle) {
			dc.DrawBitmap (*_subtitle->bitmap, _subtitle->transformed_area.x, _subtitle->transformed_area.y, true);
		}
	}

	/** Handle a size event */
	void size_event (wxSizeEvent &)
	{
		if (!_image) {
			return;
		}

		recompose ();
	}

	/** @param n Thumbnail index */
	void set (int n)
	{
		_index = n;
		_frame_rebuild_needed = true;
		Refresh ();
	}

	void set_film (Film* f)
	{
		_film = f;
		if (!_film) {
			clear ();
			_frame_rebuild_needed = true;
			Refresh ();
		} else {
			_frame_rebuild_needed = true;
			Refresh ();
		}
	}

	/** Clear our thumbnail image */
	void clear ()
	{
		_bitmap.reset ();
		_image.reset ();
		_subtitle.reset ();
	}

	void recompose ()
	{
		_composition_needed = true;
		Refresh ();
	}

	DECLARE_EVENT_TABLE ();

private:

	void compose ()
	{
		_composition_needed = false;
		
		if (!_film || !_image) {
			return;
		}

		/* Size of the view */
		int vw, vh;
		GetSize (&vw, &vh);

		/* Cropped rectangle */
		Rectangle cropped_area (
			_film->crop().left,
			_film->crop().top,
			_image->GetWidth() - (_film->crop().left + _film->crop().right),
			_image->GetHeight() - (_film->crop().top + _film->crop().bottom)
			);

		/* Target ratio */
		float const target = _film->format() ? _film->format()->ratio_as_float (_film) : 1.78;

		_transformed_image = _image->GetSubImage (wxRect (cropped_area.x, cropped_area.y, cropped_area.w, cropped_area.h));

		float x_scale = 1;
		float y_scale = 1;

		if ((float (vw) / vh) > target) {
			/* view is longer (horizontally) than the ratio; fit height */
			_transformed_image.Rescale (vh * target, vh, wxIMAGE_QUALITY_HIGH);
			x_scale = vh * target / cropped_area.w;
			y_scale = float (vh) / cropped_area.h;
		} else {
			/* view is shorter (horizontally) than the ratio; fit width */
			_transformed_image.Rescale (vw, vw / target, wxIMAGE_QUALITY_HIGH);
			x_scale = float (vw) / cropped_area.w;
			y_scale = (vw / target) / cropped_area.h;
		}

		_bitmap.reset (new wxBitmap (_transformed_image));

		if (_subtitle) {

			_subtitle->transformed_area = subtitle_transformed_area (
				x_scale, y_scale, _subtitle->base_area,	_film->subtitle_offset(), _film->subtitle_scale()
				);

			_subtitle->transformed_image = _subtitle->base_image;
			_subtitle->transformed_image.Rescale (_subtitle->transformed_area.w, _subtitle->transformed_area.h, wxIMAGE_QUALITY_HIGH);
			_subtitle->transformed_area.x -= _film->crop().left;
			_subtitle->transformed_area.y -= _film->crop().top;
			_subtitle->bitmap.reset (new wxBitmap (_subtitle->transformed_image));
		}
	}

	Film* _film;
	shared_ptr<wxImage> _image;
	wxImage _transformed_image;
	/** currently-displayed thumbnail index */
	int _index;
	shared_ptr<wxBitmap> _bitmap;
	bool _frame_rebuild_needed;
	bool _composition_needed;

	struct SubtitleView
	{
		SubtitleView (Position p, wxString const & i)
			: base_image (i)
		{
			base_area.x = p.x;
			base_area.y = p.y;
			base_area.w = base_image.GetWidth ();
			base_area.h = base_image.GetHeight ();
		}

		Rectangle base_area;
		Rectangle transformed_area;
		wxImage base_image;
		wxImage transformed_image;
		shared_ptr<wxBitmap> bitmap;
	};

	shared_ptr<SubtitleView> _subtitle;
};

BEGIN_EVENT_TABLE (ThumbPanel, wxPanel)
EVT_PAINT (ThumbPanel::paint_event)
EVT_SIZE (ThumbPanel::size_event)
END_EVENT_TABLE ()

FilmViewer::FilmViewer (Film* f, wxWindow* p)
	: wxPanel (p)
	, _film (0)
{
	_sizer = new wxBoxSizer (wxVERTICAL);
	SetSizer (_sizer);
	
	_thumb_panel = new ThumbPanel (this, f);
	_sizer->Add (_thumb_panel, 1, wxEXPAND);

	int const max = f ? f->num_thumbs() - 1 : 0;
	_slider = new wxSlider (this, wxID_ANY, 0, 0, max);
	_sizer->Add (_slider, 0, wxEXPAND | wxLEFT | wxRIGHT);
	set_thumbnail (0);

	_slider->Connect (wxID_ANY, wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler (FilmViewer::slider_changed), 0, this);

	set_film (_film);
}

void
FilmViewer::set_thumbnail (int n)
{
	if (_film == 0 || _film->num_thumbs() <= n) {
		return;
	}

	_thumb_panel->set (n);
}

void
FilmViewer::slider_changed (wxCommandEvent &)
{
	set_thumbnail (_slider->GetValue ());
}

void
FilmViewer::film_changed (Film::Property p)
{
	switch (p) {
	case Film::THUMBS:
		if (_film && _film->num_thumbs() > 1) {
			_slider->SetRange (0, _film->num_thumbs () - 1);
		} else {
			_thumb_panel->clear ();
			_slider->SetRange (0, 1);
		}
		
		_slider->SetValue (0);
		set_thumbnail (0);
		break;
	case Film::CONTENT:
		setup_visibility ();
		_film->examine_content ();
		update_thumbs ();
		break;
	case Film::CROP:
	case Film::FORMAT:
	case Film::WITH_SUBTITLES:
	case Film::SUBTITLE_OFFSET:
	case Film::SUBTITLE_SCALE:
		_thumb_panel->recompose ();
		break;
	default:
		break;
	}
}

void
FilmViewer::set_film (Film* f)
{
	if (_film == f) {
		return;
	}
	
	_film = f;
	_thumb_panel->set_film (_film);

	if (!_film) {
		return;
	}

	_film->Changed.connect (sigc::mem_fun (*this, &FilmViewer::film_changed));
	film_changed (Film::CROP);
	film_changed (Film::THUMBS);
	setup_visibility ();
}

void
FilmViewer::update_thumbs ()
{
	if (!_film) {
		return;
	}

	_film->update_thumbs_pre_gui ();

	shared_ptr<const FilmState> s = _film->state_copy ();
	shared_ptr<Options> o (new Options (s->dir ("thumbs"), ".png", ""));
	o->out_size = _film->size ();
	o->apply_crop = false;
	o->decode_audio = false;
	o->decode_video_frequency = 128;
	o->decode_subtitles = true;
	
	shared_ptr<Job> j (new ThumbsJob (s, o, _film->log(), shared_ptr<Job> ()));
	j->Finished.connect (sigc::mem_fun (_film, &Film::update_thumbs_post_gui));
	JobManager::instance()->add (j);
}

void
FilmViewer::setup_visibility ()
{
	if (!_film) {
		return;
	}

	ContentType const c = _film->content_type ();
	_slider->Show (c == VIDEO);
}
