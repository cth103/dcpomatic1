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

#include <wx/spinctrl.h>
#include "lib/filter.h"
#include "lib/ffmpeg_content.h"
#include "lib/colour_conversion.h"
#include "lib/config.h"
#include "lib/util.h"
#include "lib/ratio.h"
#include "lib/frame_rate_change.h"
#include "filter_dialog.h"
#include "video_panel.h"
#include "wx_util.h"
#include "film_editor.h"
#include "content_colour_conversion_dialog.h"
#include "content_widget.h"

using std::vector;
using std::string;
using std::pair;
using std::cout;
using std::list;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using boost::bind;
using boost::optional;

static VideoContentScale
index_to_scale (int n)
{
	vector<VideoContentScale> scales = VideoContentScale::all ();
	assert (n >= 0);
	assert (n < int (scales.size ()));
	return scales[n];
}

static int
scale_to_index (VideoContentScale scale)
{
	vector<VideoContentScale> scales = VideoContentScale::all ();
	for (size_t i = 0; i < scales.size(); ++i) {
		if (scales[i] == scale) {
			return i;
		}
	}

	assert (false);
}

VideoPanel::VideoPanel (FilmEditor* e)
	: FilmEditorPanel (e, _("Video"))
{
	wxGridBagSizer* grid = new wxGridBagSizer (DCPOMATIC_SIZER_X_GAP, DCPOMATIC_SIZER_Y_GAP);
	_sizer->Add (grid, 0, wxALL, 8);

	int r = 0;

	add_label_to_grid_bag_sizer (grid, this, _("Type"), true, wxGBPosition (r, 0));
	_frame_type = new ContentChoice<VideoContent, VideoFrameType> (
		this,
		new wxChoice (this, wxID_ANY),
		VideoContentProperty::VIDEO_FRAME_TYPE,
		boost::mem_fn (&VideoContent::video_frame_type),
		boost::mem_fn (&VideoContent::set_video_frame_type),
		&caster<int, VideoFrameType>,
		&caster<VideoFrameType, int>
		);
	_frame_type->add (grid, wxGBPosition (r, 1));
	++r;
	
	add_label_to_grid_bag_sizer (grid, this, _("Left crop"), true, wxGBPosition (r, 0));
	_left_crop = new ContentSpinCtrl<VideoContent> (
		this,
		new wxSpinCtrl (this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize (64, -1)),
		VideoContentProperty::VIDEO_CROP,
		boost::mem_fn (&VideoContent::left_crop),
		boost::mem_fn (&VideoContent::set_left_crop)
		);
	_left_crop->add (grid, wxGBPosition (r, 1));
	++r;

	add_label_to_grid_bag_sizer (grid, this, _("Right crop"), true, wxGBPosition (r, 0));
	_right_crop = new ContentSpinCtrl<VideoContent> (
		this,
		new wxSpinCtrl (this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize (64, -1)),
		VideoContentProperty::VIDEO_CROP,
		boost::mem_fn (&VideoContent::right_crop),
		boost::mem_fn (&VideoContent::set_right_crop)
		);
	_right_crop->add (grid, wxGBPosition (r, 1));
	++r;
	
	add_label_to_grid_bag_sizer (grid, this, _("Top crop"), true, wxGBPosition (r, 0));
	_top_crop = new ContentSpinCtrl<VideoContent> (
		this,
		new wxSpinCtrl (this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize (64, -1)),
		VideoContentProperty::VIDEO_CROP,
		boost::mem_fn (&VideoContent::top_crop),
		boost::mem_fn (&VideoContent::set_top_crop)
		);
	_top_crop->add (grid, wxGBPosition (r,1 ));
	++r;
	
	add_label_to_grid_bag_sizer (grid, this, _("Bottom crop"), true, wxGBPosition (r, 0));
	_bottom_crop = new ContentSpinCtrl<VideoContent> (
		this,
		new wxSpinCtrl (this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize (64, -1)),
		VideoContentProperty::VIDEO_CROP,
		boost::mem_fn (&VideoContent::bottom_crop),
		boost::mem_fn (&VideoContent::set_bottom_crop)
		);
	_bottom_crop->add (grid, wxGBPosition (r, 1));
	++r;

	add_label_to_grid_bag_sizer (grid, this, _("Scale to"), true, wxGBPosition (r, 0));
	_scale = new ContentChoice<VideoContent, VideoContentScale> (
		this,
		new wxChoice (this, wxID_ANY),
		VideoContentProperty::VIDEO_SCALE,
		boost::mem_fn (&VideoContent::scale),
		boost::mem_fn (&VideoContent::set_scale),
		&index_to_scale,
		&scale_to_index
		);
	_scale->add (grid, wxGBPosition (r, 1));
	++r;

	{
		add_label_to_grid_bag_sizer (grid, this, _("Filters"), true, wxGBPosition (r, 0));
		wxSizer* s = new wxBoxSizer (wxHORIZONTAL);

		wxClientDC dc (this);
		wxSize size = dc.GetTextExtent (wxT ("A quite long-ish name"));
		size.SetHeight (-1);
		
		_filters = new wxStaticText (this, wxID_ANY, _("None"), wxDefaultPosition, size);
		s->Add (_filters, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM | wxRIGHT, 6);
		_filters_button = new wxButton (this, wxID_ANY, _("Edit..."));
		s->Add (_filters_button, 0, wxALIGN_CENTER_VERTICAL);
		grid->Add (s, wxGBPosition (r, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	}
	++r;
	
	{
		_enable_colour_conversion = new wxCheckBox (this, wxID_ANY, _("Colour conversion"));
		grid->Add (_enable_colour_conversion, wxGBPosition (r, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);

		wxSizer* s = new wxBoxSizer (wxHORIZONTAL);

		wxClientDC dc (this);
		wxSize size = dc.GetTextExtent (wxT ("A quite long-ish name"));
		size.SetHeight (-1);
		
		_colour_conversion = new wxStaticText (this, wxID_ANY, wxT (""), wxDefaultPosition, size);

		s->Add (_colour_conversion, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM | wxRIGHT, 6);
		_colour_conversion_button = new wxButton (this, wxID_ANY, _("Edit..."));
		s->Add (_colour_conversion_button, 0, wxALIGN_CENTER_VERTICAL);
		grid->Add (s, wxGBPosition (r, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	}
	++r;

	_description = new wxStaticText (this, wxID_ANY, wxT ("\n \n \n \n \n"), wxDefaultPosition, wxDefaultSize);
	grid->Add (_description, wxGBPosition (r, 0), wxGBSpan (1, 2), wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 6);
	wxFont font = _description->GetFont();
	font.SetStyle(wxFONTSTYLE_ITALIC);
	font.SetPointSize(font.GetPointSize() - 1);
	_description->SetFont(font);
	++r;

	_left_crop->wrapped()->SetRange (0, 1024);
	_top_crop->wrapped()->SetRange (0, 1024);
	_right_crop->wrapped()->SetRange (0, 1024);
	_bottom_crop->wrapped()->SetRange (0, 1024);

	vector<VideoContentScale> scales = VideoContentScale::all ();
	_scale->wrapped()->Clear ();
	for (vector<VideoContentScale>::iterator i = scales.begin(); i != scales.end(); ++i) {
		_scale->wrapped()->Append (std_to_wx (i->name ()));
	}

	_frame_type->wrapped()->Append (_("2D"));
	_frame_type->wrapped()->Append (_("3D left/right"));
	_frame_type->wrapped()->Append (_("3D top/bottom"));
	_frame_type->wrapped()->Append (_("3D alternate"));
	_frame_type->wrapped()->Append (_("3D left only"));
	_frame_type->wrapped()->Append (_("3D right only"));

	_filters_button->Bind           (wxEVT_COMMAND_BUTTON_CLICKED,   boost::bind (&VideoPanel::edit_filters_clicked, this));
	_enable_colour_conversion->Bind (wxEVT_COMMAND_CHECKBOX_CLICKED, boost::bind (&VideoPanel::enable_colour_conversion_clicked, this));
	_colour_conversion_button->Bind (wxEVT_COMMAND_BUTTON_CLICKED,   boost::bind (&VideoPanel::edit_colour_conversion_clicked, this));
}

void
VideoPanel::film_changed (Film::Property property)
{
	switch (property) {
	case Film::CONTAINER:
	case Film::VIDEO_FRAME_RATE:
	case Film::RESOLUTION:
		setup_description ();
		break;
	default:
		break;
	}
}

void
VideoPanel::film_content_changed (int property)
{
	VideoContentList vc = _editor->selected_video_content ();
	shared_ptr<VideoContent> vcs;
	shared_ptr<FFmpegContent> fcs;
	if (!vc.empty ()) {
		vcs = vc.front ();
		fcs = dynamic_pointer_cast<FFmpegContent> (vcs);
	}
	
	if (property == VideoContentProperty::VIDEO_FRAME_TYPE) {
		setup_description ();
	} else if (property == VideoContentProperty::VIDEO_CROP) {
		setup_description ();
	} else if (property == VideoContentProperty::VIDEO_SCALE) {
		setup_description ();
	} else if (property == VideoContentProperty::VIDEO_FRAME_RATE) {
		setup_description ();
	} else if (property == VideoContentProperty::COLOUR_CONVERSION) {
		if (!vcs) {
			_colour_conversion->SetLabel (wxT (""));
		} else if (vcs->colour_conversion ()) {
			optional<size_t> preset = vcs->colour_conversion().get().preset ();
			vector<PresetColourConversion> cc = Config::instance()->colour_conversions ();
			_colour_conversion->SetLabel (preset ? std_to_wx (cc[preset.get()].name) : _("Custom"));
			_enable_colour_conversion->SetValue (true);
			_colour_conversion->Enable (true);
			_colour_conversion_button->Enable (true);
		} else {
			_colour_conversion->SetLabel (_("None"));
			_enable_colour_conversion->SetValue (false);
			_colour_conversion->Enable (false);
			_colour_conversion_button->Enable (false);
		}
	} else if (property == FFmpegContentProperty::FILTERS) {
		if (fcs) {
			string const p = Filter::ffmpeg_string (fcs->filters ());
			if (p.empty ()) {
				_filters->SetLabel (_("None"));
			} else {
				_filters->SetLabel (std_to_wx (p));
			}
		}
	}
}

/** Called when the `Edit filters' button has been clicked */
void
VideoPanel::edit_filters_clicked ()
{
	FFmpegContentList c = _editor->selected_ffmpeg_content ();
	if (c.size() != 1) {
		return;
	}

	FilterDialog* d = new FilterDialog (this, c.front()->filters());
	d->ActiveChanged.connect (bind (&FFmpegContent::set_filters, c.front(), _1));
	d->ShowModal ();
	d->Destroy ();
}

void
VideoPanel::setup_description ()
{
	VideoContentList vc = _editor->selected_video_content ();
	if (vc.empty ()) {
		_description->SetLabel ("");
		return;
	} else if (vc.size() > 1) {
		_description->SetLabel (_("Multiple content selected"));
		return;
	}

	string d = vc.front()->processing_description ();
	size_t lines = count (d.begin(), d.end(), '\n');

	for (int i = lines; i < 6; ++i) {
		d += "\n ";
	}

	_description->SetLabel (std_to_wx (d));
	_sizer->Layout ();
}

void
VideoPanel::edit_colour_conversion_clicked ()
{
	VideoContentList vc = _editor->selected_video_content ();
	if (vc.size() != 1) {
		return;
	}

	if (!vc.front()->colour_conversion ()) {
		return;
	}

	ColourConversion conversion = vc.front()->colour_conversion().get ();
	ContentColourConversionDialog* d = new ContentColourConversionDialog (this);
	d->set (conversion);
	d->ShowModal ();

	vc.front()->set_colour_conversion (d->get ());
	d->Destroy ();
}

void
VideoPanel::content_selection_changed ()
{
	VideoContentList video_sel = _editor->selected_video_content ();
	FFmpegContentList ffmpeg_sel = _editor->selected_ffmpeg_content ();
	
	bool const single = video_sel.size() == 1;

	_left_crop->set_content (video_sel);
	_right_crop->set_content (video_sel);
	_top_crop->set_content (video_sel);
	_bottom_crop->set_content (video_sel);
	_frame_type->set_content (video_sel);
	_scale->set_content (video_sel);

	_filters_button->Enable (single && !ffmpeg_sel.empty ());
	_colour_conversion_button->Enable (single);

	film_content_changed (VideoContentProperty::VIDEO_CROP);
	film_content_changed (VideoContentProperty::VIDEO_FRAME_RATE);
	film_content_changed (VideoContentProperty::COLOUR_CONVERSION);
	film_content_changed (FFmpegContentProperty::FILTERS);
}

void
VideoPanel::enable_colour_conversion_clicked ()
{
	VideoContentList vc = _editor->selected_video_content ();
	if (vc.size() != 1) {
		return;
	}

	if (_enable_colour_conversion->GetValue()) {
		vc.front()->set_default_colour_conversion ();
	} else {
		vc.front()->unset_colour_conversion ();
	}
}
