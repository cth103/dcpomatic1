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
 *  @brief A wx widget to edit a film's metadata, and perform various functions.
 */

#include <iostream>
#include <iomanip>
#include <wx/wx.h>
#include <wx/notebook.h>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include "lib/format.h"
#include "lib/film.h"
#include "lib/transcode_job.h"
#include "lib/exceptions.h"
#include "lib/ab_transcode_job.h"
#include "lib/job_manager.h"
#include "lib/filter.h"
#include "lib/config.h"
#include "lib/ffmpeg_decoder.h"
#include "lib/log.h"
#include "filter_dialog.h"
#include "wx_util.h"
#include "film_editor.h"
#include "gain_calculator_dialog.h"
#include "sound_processor.h"
#include "dci_metadata_dialog.h"
#include "scaler.h"
#include "audio_dialog.h"

using std::string;
using std::cout;
using std::stringstream;
using std::pair;
using std::fixed;
using std::setprecision;
using std::list;
using std::vector;
using std::max;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;

/** @param f Film to edit */
FilmEditor::FilmEditor (shared_ptr<Film> f, wxWindow* parent)
	: wxPanel (parent)
	, _film (f)
	, _generally_sensitive (true)
	, _audio_dialog (0)
	, _ignore_minimum_audio_channels_change (false)
{
	wxBoxSizer* s = new wxBoxSizer (wxVERTICAL);
	SetSizer (s);
	_notebook = new wxNotebook (this, wxID_ANY);
	s->Add (_notebook, 1);

	make_film_panel ();
	_notebook->AddPage (_film_panel, _("Film"), true);
	make_video_panel ();
	_notebook->AddPage (_video_panel, _("Video"), false);
	make_audio_panel ();
	_notebook->AddPage (_audio_panel, _("Audio"), false);
	make_subtitle_panel ();
	_notebook->AddPage (_subtitle_panel, _("Subtitles"), false);

	set_film (_film);
	connect_to_widgets ();

	JobManager::instance()->ActiveJobsChanged.connect (
		bind (&FilmEditor::active_jobs_changed, this, _1)
		);
	
	setup_visibility ();
	setup_formats ();
}

void
FilmEditor::make_film_panel ()
{
	_film_panel = new wxPanel (_notebook);
	_film_sizer = new wxBoxSizer (wxVERTICAL);
	_film_panel->SetSizer (_film_sizer);

	wxGridBagSizer* grid = new wxGridBagSizer (DVDOMATIC_SIZER_X_GAP, DVDOMATIC_SIZER_Y_GAP);
	_film_sizer->Add (grid, 0, wxALL, 8);

	int r = 0;
	
	add_label_to_grid_bag_sizer (grid, _film_panel, _("Name"), true, wxGBPosition (r, 0));
	_name = new wxTextCtrl (_film_panel, wxID_ANY);
	grid->Add (_name, wxGBPosition(r, 1), wxDefaultSpan, wxEXPAND | wxLEFT | wxRIGHT);
	++r;
	
	add_label_to_grid_bag_sizer (grid, _film_panel, _("DCP Name"), true, wxGBPosition (r, 0));
	_dcp_name = new wxStaticText (_film_panel, wxID_ANY, wxT (""));
	grid->Add (_dcp_name, wxGBPosition(r, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	++r;

	int flags = wxALIGN_CENTER_VERTICAL;
#ifdef __WXOSX__
	flags |= wxALIGN_RIGHT;
#endif	
	
	_use_dci_name = new wxCheckBox (_film_panel, wxID_ANY, _("Use DCI name"));
	grid->Add (_use_dci_name, wxGBPosition (r, 0), wxDefaultSpan, flags);
	_edit_dci_button = new wxButton (_film_panel, wxID_ANY, _("Details..."));
	grid->Add (_edit_dci_button, wxGBPosition (r, 1), wxDefaultSpan);
	++r;

	add_label_to_grid_bag_sizer (grid, _film_panel, _("Content"), true, wxGBPosition (r, 0));
	_content = new wxFilePickerCtrl (_film_panel, wxID_ANY, wxT (""), _("Select Content File"), wxT("*.*"));
	grid->Add (_content, wxGBPosition (r, 1), wxDefaultSpan, wxEXPAND);
	++r;

	_trust_content_header = new wxCheckBox (_film_panel, wxID_ANY, _("Trust content's header"));
	video_control (_trust_content_header);
	grid->Add (_trust_content_header, wxGBPosition (r, 0), wxGBSpan(1, 2));
	++r;

	add_label_to_grid_bag_sizer (grid, _film_panel, _("Content Type"), true, wxGBPosition (r, 0));
	_dcp_content_type = new wxChoice (_film_panel, wxID_ANY);
	grid->Add (_dcp_content_type, wxGBPosition (r, 1));
	++r;

	video_control (add_label_to_grid_bag_sizer (grid, _film_panel, _("Original Frame Rate"), true, wxGBPosition (r, 0)));
	_source_frame_rate = new wxStaticText (_film_panel, wxID_ANY, wxT (""));
	grid->Add (video_control (_source_frame_rate), wxGBPosition (r, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	++r;

	{
		add_label_to_grid_bag_sizer (grid, _film_panel, _("DCP Frame Rate"), true, wxGBPosition (r, 0));
		wxBoxSizer* s = new wxBoxSizer (wxHORIZONTAL);
		_dcp_frame_rate = new wxChoice (_film_panel, wxID_ANY);
		s->Add (_dcp_frame_rate, 1, wxALIGN_CENTER_VERTICAL);
		_best_dcp_frame_rate = new wxButton (_film_panel, wxID_ANY, _("Use best"));
		s->Add (_best_dcp_frame_rate, 1, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 6);
		grid->Add (s, wxGBPosition (r, 1));
	}
	++r;

	_frame_rate_description = new wxStaticText (_film_panel, wxID_ANY, wxT ("\n \n "), wxDefaultPosition, wxDefaultSize);
	grid->Add (video_control (_frame_rate_description), wxGBPosition (r, 0), wxGBSpan (1, 2), wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 6);
	{
		wxFont font = _frame_rate_description->GetFont();
		font.SetStyle(wxFONTSTYLE_ITALIC);
		font.SetPointSize(font.GetPointSize() - 1);
		_frame_rate_description->SetFont(font);
	}
	++r;
	
	video_control (add_label_to_grid_bag_sizer (grid, _film_panel, _("Length"), true, wxGBPosition (r, 0)));
	_length = new wxStaticText (_film_panel, wxID_ANY, wxT (""));
	grid->Add (video_control (_length), wxGBPosition (r, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	++r;


	{
		video_control (add_label_to_grid_bag_sizer (grid, _film_panel, _("Trim frames"), true, wxGBPosition (r, 0)));
		wxBoxSizer* s = new wxBoxSizer (wxHORIZONTAL);
		video_control (add_label_to_sizer (s, _film_panel, _("Start"), true));
		_trim_start = new wxSpinCtrl (_film_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize (64, -1));
		s->Add (video_control (_trim_start));
		video_control (add_label_to_sizer (s, _film_panel, _("End"), true));
		_trim_end = new wxSpinCtrl (_film_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize (64, -1));
		s->Add (video_control (_trim_end));

		grid->Add (s, wxGBPosition (r, 1));
	}
	++r;

	video_control (add_label_to_grid_bag_sizer (grid, _film_panel, _("Trim method"), true, wxGBPosition (r, 0)));
	_trim_type = new wxChoice (_film_panel, wxID_ANY);
	grid->Add (video_control (_trim_type), wxGBPosition (r, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	++r;

	_dcp_ab = new wxCheckBox (_film_panel, wxID_ANY, _("A/B"));
	video_control (_dcp_ab);
	grid->Add (_dcp_ab, wxGBPosition (r, 0));
	++r;

	/* STILL-only stuff */
	{
		still_control (add_label_to_grid_bag_sizer (grid, _film_panel, _("Duration"), true, wxGBPosition (r, 0)));
		wxSizer* s = new wxBoxSizer (wxHORIZONTAL);
		_still_duration = new wxSpinCtrl (_film_panel);
		still_control (_still_duration);
		s->Add (_still_duration, 1, wxEXPAND);
		/// TRANSLATORS: `s' here is an abbreviation for seconds, the unit of time
		still_control (add_label_to_sizer (s, _film_panel, _("s"), false));
		grid->Add (s, wxGBPosition (r, 1));
	}
	++r;

	_warnings = new wxStaticText (_film_panel, wxID_ANY, wxT ("\n \n \n \n"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
	grid->Add (_warnings, wxGBPosition (r, 0), wxGBSpan (1, 2), wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 6);
	{
		wxFont font = _warnings->GetFont();
		font.SetWeight(wxFONTWEIGHT_BOLD);
		font.SetPointSize(font.GetPointSize() - 1);
		_warnings->SetFont (font);
	}
	++r;
	
	vector<DCPContentType const *> const ct = DCPContentType::all ();
	for (vector<DCPContentType const *>::const_iterator i = ct.begin(); i != ct.end(); ++i) {
		_dcp_content_type->Append (std_to_wx ((*i)->pretty_name ()));
	}

	list<int> const dfr = Config::instance()->allowed_dcp_frame_rates ();
	for (list<int>::const_iterator i = dfr.begin(); i != dfr.end(); ++i) {
		_dcp_frame_rate->Append (std_to_wx (boost::lexical_cast<string> (*i)));
	}

	_trim_type->Append (_("encode all frames and play the subset"));
	_trim_type->Append (_("encode only the subset"));
}

void
FilmEditor::connect_to_widgets ()
{
	_name->Connect                 (wxID_ANY, wxEVT_COMMAND_TEXT_UPDATED,         wxCommandEventHandler (FilmEditor::name_changed), 0, this);
	_use_dci_name->Connect         (wxID_ANY, wxEVT_COMMAND_CHECKBOX_CLICKED,     wxCommandEventHandler (FilmEditor::use_dci_name_toggled), 0, this);
	_edit_dci_button->Connect      (wxID_ANY, wxEVT_COMMAND_BUTTON_CLICKED,       wxCommandEventHandler (FilmEditor::edit_dci_button_clicked), 0, this);
	_format->Connect               (wxID_ANY, wxEVT_COMMAND_CHOICE_SELECTED,      wxCommandEventHandler (FilmEditor::format_changed), 0, this);
	_content->Connect              (wxID_ANY, wxEVT_COMMAND_FILEPICKER_CHANGED,   wxCommandEventHandler (FilmEditor::content_changed), 0, this);
	_trust_content_header->Connect (wxID_ANY, wxEVT_COMMAND_CHECKBOX_CLICKED,     wxCommandEventHandler (FilmEditor::trust_content_header_changed), 0, this);
	_left_crop->Connect            (wxID_ANY, wxEVT_COMMAND_SPINCTRL_UPDATED,     wxCommandEventHandler (FilmEditor::left_crop_changed), 0, this);
	_right_crop->Connect           (wxID_ANY, wxEVT_COMMAND_SPINCTRL_UPDATED,     wxCommandEventHandler (FilmEditor::right_crop_changed), 0, this);
	_top_crop->Connect             (wxID_ANY, wxEVT_COMMAND_SPINCTRL_UPDATED,     wxCommandEventHandler (FilmEditor::top_crop_changed), 0, this);
	_bottom_crop->Connect          (wxID_ANY, wxEVT_COMMAND_SPINCTRL_UPDATED,     wxCommandEventHandler (FilmEditor::bottom_crop_changed), 0, this);
	_filters_button->Connect       (wxID_ANY, wxEVT_COMMAND_BUTTON_CLICKED,       wxCommandEventHandler (FilmEditor::edit_filters_clicked), 0, this);
	_scaler->Connect               (wxID_ANY, wxEVT_COMMAND_CHOICE_SELECTED,      wxCommandEventHandler (FilmEditor::scaler_changed), 0, this);
	_dcp_content_type->Connect     (wxID_ANY, wxEVT_COMMAND_CHOICE_SELECTED,      wxCommandEventHandler (FilmEditor::dcp_content_type_changed), 0, this);
	_dcp_frame_rate->Connect       (wxID_ANY, wxEVT_COMMAND_CHOICE_SELECTED,      wxCommandEventHandler (FilmEditor::dcp_frame_rate_changed), 0, this);
	_best_dcp_frame_rate->Connect  (wxID_ANY, wxEVT_COMMAND_BUTTON_CLICKED,       wxCommandEventHandler (FilmEditor::best_dcp_frame_rate_clicked), 0, this);
	_pad_with_silence->Connect     (wxID_ANY, wxEVT_COMMAND_CHECKBOX_CLICKED,     wxCommandEventHandler (FilmEditor::pad_with_silence_toggled), 0, this);
	_minimum_audio_channels->Connect (wxID_ANY, wxEVT_COMMAND_SPINCTRL_UPDATED,   wxCommandEventHandler (FilmEditor::minimum_audio_channels_changed), 0, this);
	_dcp_ab->Connect               (wxID_ANY, wxEVT_COMMAND_CHECKBOX_CLICKED,     wxCommandEventHandler (FilmEditor::dcp_ab_toggled), 0, this);
	_still_duration->Connect       (wxID_ANY, wxEVT_COMMAND_SPINCTRL_UPDATED,     wxCommandEventHandler (FilmEditor::still_duration_changed), 0, this);
	_trim_start->Connect           (wxID_ANY, wxEVT_COMMAND_SPINCTRL_UPDATED,     wxCommandEventHandler (FilmEditor::trim_start_changed), 0, this);
	_trim_end->Connect             (wxID_ANY, wxEVT_COMMAND_SPINCTRL_UPDATED,     wxCommandEventHandler (FilmEditor::trim_end_changed), 0, this);
	_trim_type->Connect            (wxID_ANY, wxEVT_COMMAND_CHOICE_SELECTED,      wxCommandEventHandler (FilmEditor::trim_type_changed), 0, this);
	_with_subtitles->Connect       (wxID_ANY, wxEVT_COMMAND_CHECKBOX_CLICKED,     wxCommandEventHandler (FilmEditor::with_subtitles_toggled), 0, this);
	_subtitle_offset->Connect      (wxID_ANY, wxEVT_COMMAND_SPINCTRL_UPDATED,     wxCommandEventHandler (FilmEditor::subtitle_offset_changed), 0, this);
	_subtitle_scale->Connect       (wxID_ANY, wxEVT_COMMAND_SPINCTRL_UPDATED,     wxCommandEventHandler (FilmEditor::subtitle_scale_changed), 0, this);
	_colour_lut->Connect           (wxID_ANY, wxEVT_COMMAND_CHOICE_SELECTED,      wxCommandEventHandler (FilmEditor::colour_lut_changed), 0, this);
	_j2k_bandwidth->Connect        (wxID_ANY, wxEVT_COMMAND_SPINCTRL_UPDATED,     wxCommandEventHandler (FilmEditor::j2k_bandwidth_changed), 0, this);
	_subtitle_stream->Connect      (wxID_ANY, wxEVT_COMMAND_CHOICE_SELECTED,      wxCommandEventHandler (FilmEditor::subtitle_stream_changed), 0, this);
	_audio_stream->Connect         (wxID_ANY, wxEVT_COMMAND_CHOICE_SELECTED,      wxCommandEventHandler (FilmEditor::audio_stream_changed), 0, this);
	_audio_gain->Connect           (wxID_ANY, wxEVT_COMMAND_SPINCTRL_UPDATED,     wxCommandEventHandler (FilmEditor::audio_gain_changed), 0, this);
	_audio_gain_calculate_button->Connect (
		wxID_ANY, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler (FilmEditor::audio_gain_calculate_button_clicked), 0, this
		);
	_show_audio->Connect           (wxID_ANY, wxEVT_COMMAND_BUTTON_CLICKED,       wxCommandEventHandler (FilmEditor::show_audio_clicked), 0, this);
	_audio_delay->Connect          (wxID_ANY, wxEVT_COMMAND_SPINCTRL_UPDATED,     wxCommandEventHandler (FilmEditor::audio_delay_changed), 0, this);
	_use_content_audio->Connect    (wxID_ANY, wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler (FilmEditor::use_audio_changed), 0, this);
	_use_external_audio->Connect   (wxID_ANY, wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler (FilmEditor::use_audio_changed), 0, this);
	for (int i = 0; i < MAX_AUDIO_CHANNELS; ++i) {
		_external_audio[i]->Connect (
			wxID_ANY, wxEVT_COMMAND_FILEPICKER_CHANGED, wxCommandEventHandler (FilmEditor::external_audio_changed), 0, this
			);
	}
}

void
FilmEditor::make_video_panel ()
{
	_video_panel = new wxPanel (_notebook);
	_video_sizer = new wxBoxSizer (wxVERTICAL);
	_video_panel->SetSizer (_video_sizer);
	
	wxGridBagSizer* grid = new wxGridBagSizer (DVDOMATIC_SIZER_X_GAP, DVDOMATIC_SIZER_Y_GAP);
	_video_sizer->Add (grid, 0, wxALL, 8);

	int r = 0;
	add_label_to_grid_bag_sizer (grid, _video_panel, _("Format"), true, wxGBPosition (r, 0));
	_format = new wxChoice (_video_panel, wxID_ANY);
	grid->Add (_format, wxGBPosition (r, 1));
	++r;

	add_label_to_grid_bag_sizer (grid, _video_panel, _("Left crop"), true, wxGBPosition (r, 0));
	_left_crop = new wxSpinCtrl (_video_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize (64, -1));
	grid->Add (_left_crop, wxGBPosition (r, 1));
	++r;

	add_label_to_grid_bag_sizer (grid, _video_panel, _("Right crop"), true, wxGBPosition (r, 0));
	_right_crop = new wxSpinCtrl (_video_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize (64, -1));
	grid->Add (_right_crop, wxGBPosition (r, 1));
	++r;
	
	add_label_to_grid_bag_sizer (grid, _video_panel, _("Top crop"), true, wxGBPosition (r, 0));
	_top_crop = new wxSpinCtrl (_video_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize (64, -1));
	grid->Add (_top_crop, wxGBPosition (r, 1));
	++r;
	
	add_label_to_grid_bag_sizer (grid, _video_panel, _("Bottom crop"), true, wxGBPosition (r, 0));
	_bottom_crop = new wxSpinCtrl (_video_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize (64, -1));
	grid->Add (_bottom_crop, wxGBPosition (r, 1));
	++r;

	_scaling_description = new wxStaticText (_video_panel, wxID_ANY, wxT ("\n \n \n \n"), wxDefaultPosition, wxDefaultSize);
	grid->Add (_scaling_description, wxGBPosition (r, 0), wxGBSpan (1, 2), wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 6);
	wxFont font = _scaling_description->GetFont();
	font.SetStyle(wxFONTSTYLE_ITALIC);
	font.SetPointSize(font.GetPointSize() - 1);
	_scaling_description->SetFont(font);
	++r;

	/* VIDEO-only stuff */
	{
		video_control (add_label_to_grid_bag_sizer (grid, _video_panel, _("Filters"), true, wxGBPosition (r, 0)));
		wxSizer* s = new wxBoxSizer (wxHORIZONTAL);
		_filters = new wxStaticText (_video_panel, wxID_ANY, _("None"));
		video_control (_filters);
		s->Add (_filters, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM | wxRIGHT, 6);
		_filters_button = new wxButton (_video_panel, wxID_ANY, _("Edit..."));
		video_control (_filters_button);
		s->Add (_filters_button, 0, wxALIGN_CENTER_VERTICAL);
		grid->Add (s, wxGBPosition (r, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	}
	++r;

	video_control (add_label_to_grid_bag_sizer (grid, _video_panel, _("Scaler"), true, wxGBPosition (r, 0)));
	_scaler = new wxChoice (_video_panel, wxID_ANY);
	grid->Add (video_control (_scaler), wxGBPosition (r, 1));
	++r;

	vector<Scaler const *> const sc = Scaler::all ();
	for (vector<Scaler const *>::const_iterator i = sc.begin(); i != sc.end(); ++i) {
		_scaler->Append (std_to_wx ((*i)->name()));
	}

	add_label_to_grid_bag_sizer (grid, _video_panel, _("Colour look-up table"), true, wxGBPosition (r, 0));
	_colour_lut = new wxChoice (_video_panel, wxID_ANY);
	for (int i = 0; i < 2; ++i) {
		_colour_lut->Append (std_to_wx (colour_lut_index_to_name (i)));
	}
	_colour_lut->SetSelection (0);
	grid->Add (_colour_lut, wxGBPosition (r, 1), wxDefaultSpan, wxEXPAND);
	++r;

	{
		add_label_to_grid_bag_sizer (grid, _video_panel, _("JPEG2000 bandwidth"), true, wxGBPosition (r, 0));
		wxSizer* s = new wxBoxSizer (wxHORIZONTAL);
		_j2k_bandwidth = new wxSpinCtrl (_video_panel, wxID_ANY);
		s->Add (_j2k_bandwidth, 1);
		add_label_to_sizer (s, _video_panel, _("MBps"), false);
		grid->Add (s, wxGBPosition (r, 1));
	}
	++r;

	_left_crop->SetRange (0, 1024);
	_top_crop->SetRange (0, 1024);
	_right_crop->SetRange (0, 1024);
	_bottom_crop->SetRange (0, 1024);
	_still_duration->SetRange (1, 60 * 60);
	_trim_start->SetRange (0, 24 * 60 * 60);
	_trim_end->SetRange (0, 24 * 60 * 60);
	_j2k_bandwidth->SetRange (50, 250);
}

void
FilmEditor::make_audio_panel ()
{
	_audio_panel = new wxPanel (_notebook);
	_audio_sizer = new wxBoxSizer (wxVERTICAL);
	_audio_panel->SetSizer (_audio_sizer);
	
	wxFlexGridSizer* grid = new wxFlexGridSizer (2, DVDOMATIC_SIZER_X_GAP, DVDOMATIC_SIZER_Y_GAP);
	_audio_sizer->Add (grid, 0, wxALL, 8);

	_show_audio = new wxButton (_audio_panel, wxID_ANY, _("Show Audio..."));
	grid->Add (_show_audio, 1);
	grid->AddSpacer (0);

	{
		video_control (add_label_to_sizer (grid, _audio_panel, _("Audio Gain"), true));
		wxBoxSizer* s = new wxBoxSizer (wxHORIZONTAL);
		_audio_gain = new wxSpinCtrl (_audio_panel);
		s->Add (video_control (_audio_gain), 1);
		video_control (add_label_to_sizer (s, _audio_panel, _("dB"), false));
		_audio_gain_calculate_button = new wxButton (_audio_panel, wxID_ANY, _("Calculate..."));
		video_control (_audio_gain_calculate_button);
		s->Add (_audio_gain_calculate_button, 1, wxEXPAND);
		grid->Add (s);
	}

	{
		video_control (add_label_to_sizer (grid, _audio_panel, _("Audio Delay"), true));
		wxBoxSizer* s = new wxBoxSizer (wxHORIZONTAL);
		_audio_delay = new wxSpinCtrl (_audio_panel);
		s->Add (video_control (_audio_delay), 1);
		/// TRANSLATORS: this is an abbreviation for milliseconds, the unit of time
		video_control (add_label_to_sizer (s, _audio_panel, _("ms"), false));
		grid->Add (s);
	}

	{
		_pad_with_silence = new wxCheckBox (_audio_panel, wxID_ANY, _("Pad with silence to"));
		grid->Add (_pad_with_silence);
		wxBoxSizer* s = new wxBoxSizer (wxHORIZONTAL);
		_minimum_audio_channels = new wxSpinCtrl (_audio_panel);
		s->Add (_minimum_audio_channels, 1);
		add_label_to_sizer (s, _audio_panel, _("channels"), false);
		grid->Add (s);
	}

	{
		_use_content_audio = new wxRadioButton (_audio_panel, wxID_ANY, _("Use content's audio"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		grid->Add (video_control (_use_content_audio));
		wxBoxSizer* s = new wxBoxSizer (wxHORIZONTAL);
		_audio_stream = new wxChoice (_audio_panel, wxID_ANY);
		s->Add (video_control (_audio_stream), 1);
		_audio = new wxStaticText (_audio_panel, wxID_ANY, wxT (""));
		s->Add (video_control (_audio), 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
		grid->Add (s);
	}

	_use_external_audio = new wxRadioButton (_audio_panel, wxID_ANY, _("Use external audio"));
	grid->Add (_use_external_audio);
	grid->AddSpacer (0);

	for (int i = 0; i < MAX_AUDIO_CHANNELS; ++i) {
		add_label_to_sizer (grid, _audio_panel, std_to_wx (audio_channel_name (i)), true);
		_external_audio[i] = new wxFilePickerCtrl (_audio_panel, wxID_ANY, wxT (""), _("Select Audio File"), wxT ("*.wav"));
		grid->Add (_external_audio[i], 1, wxEXPAND);
	}

	_audio_gain->SetRange (-60, 60);
	_audio_delay->SetRange (-1000, 1000);
	_ignore_minimum_audio_channels_change = true;
	_minimum_audio_channels->SetRange (0, MAX_AUDIO_CHANNELS);
	_ignore_minimum_audio_channels_change = false;
}

void
FilmEditor::make_subtitle_panel ()
{
	_subtitle_panel = new wxPanel (_notebook);
	_subtitle_sizer = new wxBoxSizer (wxVERTICAL);
	_subtitle_panel->SetSizer (_subtitle_sizer);
	wxFlexGridSizer* grid = new wxFlexGridSizer (2, DVDOMATIC_SIZER_X_GAP, DVDOMATIC_SIZER_Y_GAP);
	_subtitle_sizer->Add (grid, 0, wxALL, 8);

	_with_subtitles = new wxCheckBox (_subtitle_panel, wxID_ANY, _("With Subtitles"));
	video_control (_with_subtitles);
	grid->Add (_with_subtitles, 1);
	
	_subtitle_stream = new wxChoice (_subtitle_panel, wxID_ANY);
	grid->Add (video_control (_subtitle_stream));

	{
		video_control (add_label_to_sizer (grid, _subtitle_panel, _("Subtitle Offset"), true));
		wxBoxSizer* s = new wxBoxSizer (wxHORIZONTAL);
		_subtitle_offset = new wxSpinCtrl (_subtitle_panel);
		s->Add (_subtitle_offset);
		video_control (add_label_to_sizer (s, _subtitle_panel, _("pixels"), false));
		grid->Add (s);
	}

	{
		video_control (add_label_to_sizer (grid, _subtitle_panel, _("Subtitle Scale"), true));
		wxBoxSizer* s = new wxBoxSizer (wxHORIZONTAL);
		_subtitle_scale = new wxSpinCtrl (_subtitle_panel);
		s->Add (video_control (_subtitle_scale));
		video_control (add_label_to_sizer (s, _subtitle_panel, _("%"), false));
		grid->Add (s);
	}

	_subtitle_offset->SetRange (-1024, 1024);
	_subtitle_scale->SetRange (1, 1000);
}

/** Called when the left crop widget has been changed */
void
FilmEditor::left_crop_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_left_crop (_left_crop->GetValue ());
}

/** Called when the right crop widget has been changed */
void
FilmEditor::right_crop_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_right_crop (_right_crop->GetValue ());
}

/** Called when the top crop widget has been changed */
void
FilmEditor::top_crop_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_top_crop (_top_crop->GetValue ());
}

/** Called when the bottom crop value has been changed */
void
FilmEditor::bottom_crop_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_bottom_crop (_bottom_crop->GetValue ());
}

/** Called when the content filename has been changed */
void
FilmEditor::content_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	try {
		_film->set_content (wx_to_std (_content->GetPath ()));
	} catch (std::exception& e) {
		_content->SetPath (std_to_wx (_film->directory ()));
		error_dialog (this, wxString::Format (_("Could not set content: %s"), std_to_wx (e.what()).data()));
	}
}

void
FilmEditor::trust_content_header_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_trust_content_header (_trust_content_header->GetValue ());
}

/** Called when the DCP A/B switch has been toggled */
void
FilmEditor::dcp_ab_toggled (wxCommandEvent &)
{
	if (!_film) {
		return;
	}
	
	_film->set_dcp_ab (_dcp_ab->GetValue ());
}

/** Called when the name widget has been changed */
void
FilmEditor::name_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_name (string (_name->GetValue().mb_str()));
}

void
FilmEditor::subtitle_offset_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_subtitle_offset (_subtitle_offset->GetValue ());
}

void
FilmEditor::subtitle_scale_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_subtitle_scale (_subtitle_scale->GetValue() / 100.0);
}

void
FilmEditor::colour_lut_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}
	
	_film->set_colour_lut (_colour_lut->GetSelection ());
}

void
FilmEditor::j2k_bandwidth_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}
	
	_film->set_j2k_bandwidth (_j2k_bandwidth->GetValue() * 1e6);
}

void
FilmEditor::dcp_frame_rate_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_dcp_frame_rate (
		boost::lexical_cast<int> (
			wx_to_std (_dcp_frame_rate->GetString (_dcp_frame_rate->GetSelection ()))
			)
		);
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

	stringstream s;
		
	switch (p) {
	case Film::NONE:
		break;
	case Film::CONTENT:
		checked_set (_content, _film->content ());
		setup_visibility ();
		setup_formats ();
		setup_subtitle_control_sensitivity ();
		setup_streams ();
		setup_show_audio_sensitivity ();
		setup_frame_rate_description ();
		setup_minimum_audio_channels ();
		break;
	case Film::TRUST_CONTENT_HEADER:
		checked_set (_trust_content_header, _film->trust_content_header ());
		break;
	case Film::SUBTITLE_STREAMS:
		setup_subtitle_control_sensitivity ();
		setup_streams ();
		break;
	case Film::CONTENT_AUDIO_STREAMS:
		setup_streams ();
		setup_show_audio_sensitivity ();
		setup_frame_rate_description ();
		setup_minimum_audio_channels ();
		break;
	case Film::FORMAT:
	{
		int n = 0;
		vector<Format const *>::iterator i = _formats.begin ();
		while (i != _formats.end() && *i != _film->format ()) {
			++i;
			++n;
		}
		if (i == _formats.end()) {
			checked_set (_format, -1);
		} else {
			checked_set (_format, n);
		}
		setup_dcp_name ();
		setup_scaling_description ();
		break;
	}
	case Film::CROP:
		checked_set (_left_crop, _film->crop().left);
		checked_set (_right_crop, _film->crop().right);
		checked_set (_top_crop, _film->crop().top);
		checked_set (_bottom_crop, _film->crop().bottom);
		setup_scaling_description ();
		break;
	case Film::FILTERS:
	{
		pair<string, string> p = Filter::ffmpeg_strings (_film->filters ());
		if (p.first.empty () && p.second.empty ()) {
			_filters->SetLabel (_("None"));
		} else {
			string const b = p.first + " " + p.second;
			_filters->SetLabel (std_to_wx (b));
		}
		_film_sizer->Layout ();
		break;
	}
	case Film::NAME:
		checked_set (_name, _film->name());
		setup_dcp_name ();
		break;
	case Film::SOURCE_FRAME_RATE:
		s << fixed << setprecision(2) << _film->source_frame_rate();
		_source_frame_rate->SetLabel (std_to_wx (s.str ()));
		setup_frame_rate_description ();
		break;
	case Film::SIZE:
		setup_scaling_description ();
		break;
	case Film::LENGTH:
		if (_film->source_frame_rate() > 0 && _film->length()) {
			s << _film->length().get() << " "
			  << wx_to_std (_("frames")) << "; " << seconds_to_hms (_film->length().get() / _film->source_frame_rate());
		} else if (_film->length()) {
			s << _film->length().get() << " "
			  << wx_to_std (_("frames"));
		} 
		_length->SetLabel (std_to_wx (s.str ()));
		if (_film->length()) {
			_trim_start->SetRange (0, _film->length().get());
			_trim_end->SetRange (0, _film->length().get());
		}
		break;
	case Film::DCP_CONTENT_TYPE:
		checked_set (_dcp_content_type, DCPContentType::as_index (_film->dcp_content_type ()));
		setup_dcp_name ();
		break;
	case Film::DCP_AB:
		checked_set (_dcp_ab, _film->dcp_ab ());
		break;
	case Film::SCALER:
		checked_set (_scaler, Scaler::as_index (_film->scaler ()));
		break;
	case Film::TRIM_START:
		checked_set (_trim_start, _film->trim_start());
		break;
	case Film::TRIM_END:
		checked_set (_trim_end, _film->trim_end());
		break;
	case Film::TRIM_TYPE:
		checked_set (_trim_type, _film->trim_type() == Film::CPL ? 0 : 1);
		break;
	case Film::AUDIO_GAIN:
		checked_set (_audio_gain, _film->audio_gain ());
		break;
	case Film::AUDIO_DELAY:
		checked_set (_audio_delay, _film->audio_delay ());
		break;
	case Film::STILL_DURATION:
		checked_set (_still_duration, _film->still_duration ());
		break;
	case Film::WITH_SUBTITLES:
		checked_set (_with_subtitles, _film->with_subtitles ());
		setup_subtitle_control_sensitivity ();
		setup_dcp_name ();
		break;
	case Film::SUBTITLE_OFFSET:
		checked_set (_subtitle_offset, _film->subtitle_offset ());
		break;
	case Film::SUBTITLE_SCALE:
		checked_set (_subtitle_scale, _film->subtitle_scale() * 100);
		break;
	case Film::COLOUR_LUT:
		checked_set (_colour_lut, _film->colour_lut ());
		break;
	case Film::J2K_BANDWIDTH:
		checked_set (_j2k_bandwidth, double (_film->j2k_bandwidth()) / 1e6);
		break;
	case Film::USE_DCI_NAME:
		checked_set (_use_dci_name, _film->use_dci_name ());
		setup_dcp_name ();
		break;
	case Film::DCI_METADATA:
		setup_dcp_name ();
		break;
	case Film::CONTENT_AUDIO_STREAM:
		if (_film->content_audio_stream()) {
			checked_set (_audio_stream, _film->content_audio_stream()->to_string());
		}
		setup_dcp_name ();
		setup_audio_details ();
		setup_audio_control_sensitivity ();
		setup_show_audio_sensitivity ();
		setup_frame_rate_description ();
		setup_minimum_audio_channels ();
		break;
	case Film::USE_CONTENT_AUDIO:
		checked_set (_use_content_audio, _film->use_content_audio());
		checked_set (_use_external_audio, !_film->use_content_audio());
		setup_dcp_name ();
		setup_audio_details ();
		setup_audio_control_sensitivity ();
		setup_show_audio_sensitivity ();
		setup_frame_rate_description ();
		setup_minimum_audio_channels ();
		break;
	case Film::SUBTITLE_STREAM:
		if (_film->subtitle_stream()) {
			checked_set (_subtitle_stream, _film->subtitle_stream()->to_string());
		}
		break;
	case Film::EXTERNAL_AUDIO:
	{
		vector<string> a = _film->external_audio ();
		for (size_t i = 0; i < a.size() && i < MAX_AUDIO_CHANNELS; ++i) {
			checked_set (_external_audio[i], a[i]);
		}
		setup_audio_details ();
		setup_show_audio_sensitivity ();
		setup_frame_rate_description ();
		setup_minimum_audio_channels ();
		break;
	}
	case Film::DCP_FRAME_RATE:
		for (unsigned int i = 0; i < _dcp_frame_rate->GetCount(); ++i) {
			if (wx_to_std (_dcp_frame_rate->GetString(i)) == boost::lexical_cast<string> (_film->dcp_frame_rate())) {
				if (_dcp_frame_rate->GetSelection() != int(i)) {
					_dcp_frame_rate->SetSelection (i);
					break;
				}
			}
		}

		if (_film->source_frame_rate()) {
			_best_dcp_frame_rate->Enable (best_dcp_frame_rate (_film->source_frame_rate ()) != _film->dcp_frame_rate ());
		} else {
			_best_dcp_frame_rate->Disable ();
		}

		setup_frame_rate_description ();
	case Film::MINIMUM_AUDIO_CHANNELS:
		checked_set (_minimum_audio_channels, _film->minimum_audio_channels ());
		setup_minimum_audio_channels ();
		break;
	}

	setup_warnings ();
}

void
FilmEditor::setup_frame_rate_description ()
{
	wxString d;
	int lines = 0;
	
	if (_film->source_frame_rate()) {
		d << std_to_wx (FrameRateConversion (_film->source_frame_rate(), _film->dcp_frame_rate()).description);
		++lines;
#ifdef HAVE_SWRESAMPLE
		if (_film->audio_stream() && _film->audio_stream()->sample_rate() != _film->target_audio_sample_rate ()) {
			d << wxString::Format (
				_("Audio will be resampled from %dHz to %dHz\n"),
				_film->audio_stream()->sample_rate(),
				_film->target_audio_sample_rate()
				);
			++lines;
		}
#endif		
	}

	for (int i = lines; i < 2; ++i) {
		d << wxT ("\n ");
	}

	_frame_rate_description->SetLabel (d);
}

/** Called when the format widget has been changed */
void
FilmEditor::format_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	int const n = _format->GetSelection ();
	if (n >= 0) {
		assert (n < int (_formats.size()));
		_film->set_format (_formats[n]);
	}
}

/** Called when the DCP content type widget has been changed */
void
FilmEditor::dcp_content_type_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	int const n = _dcp_content_type->GetSelection ();
	if (n != wxNOT_FOUND) {
		_film->set_dcp_content_type (DCPContentType::from_index (n));
	}
}

/** Sets the Film that we are editing */
void
FilmEditor::set_film (shared_ptr<Film> f)
{
	_film = f;

	set_things_sensitive (_film != 0);

	if (_film) {
		_film->Changed.connect (bind (&FilmEditor::film_changed, this, _1));
	}

	if (_film) {
		FileChanged (_film->directory ());
	} else {
		FileChanged ("");
	}

	if (_audio_dialog) {
		_audio_dialog->set_film (_film);
	}
	
	film_changed (Film::NAME);
	film_changed (Film::USE_DCI_NAME);
	film_changed (Film::CONTENT);
	film_changed (Film::TRUST_CONTENT_HEADER);
	film_changed (Film::DCP_CONTENT_TYPE);
	film_changed (Film::FORMAT);
	film_changed (Film::CROP);
	film_changed (Film::FILTERS);
	film_changed (Film::SCALER);
	film_changed (Film::TRIM_START);
	film_changed (Film::TRIM_END);
	film_changed (Film::TRIM_TYPE);
	film_changed (Film::DCP_AB);
	film_changed (Film::CONTENT_AUDIO_STREAM);
	film_changed (Film::EXTERNAL_AUDIO);
	film_changed (Film::USE_CONTENT_AUDIO);
	film_changed (Film::AUDIO_GAIN);
	film_changed (Film::AUDIO_DELAY);
	film_changed (Film::STILL_DURATION);
	film_changed (Film::WITH_SUBTITLES);
	film_changed (Film::SUBTITLE_OFFSET);
	film_changed (Film::SUBTITLE_SCALE);
	film_changed (Film::COLOUR_LUT);
	film_changed (Film::J2K_BANDWIDTH);
	film_changed (Film::DCI_METADATA);
	film_changed (Film::SIZE);
	film_changed (Film::LENGTH);
	film_changed (Film::CONTENT_AUDIO_STREAMS);
	film_changed (Film::SUBTITLE_STREAMS);
	film_changed (Film::SOURCE_FRAME_RATE);
	film_changed (Film::DCP_FRAME_RATE);
	film_changed (Film::MINIMUM_AUDIO_CHANNELS);
}

/** Updates the sensitivity of lots of widgets to a given value.
 *  @param s true to make sensitive, false to make insensitive.
 */
void
FilmEditor::set_things_sensitive (bool s)
{
	_generally_sensitive = s;
	
	_name->Enable (s);
	_use_dci_name->Enable (s);
	_edit_dci_button->Enable (s);
	_format->Enable (s);
	_content->Enable (s);
	_trust_content_header->Enable (s);
	_left_crop->Enable (s);
	_right_crop->Enable (s);
	_top_crop->Enable (s);
	_bottom_crop->Enable (s);
	_filters_button->Enable (s);
	_scaler->Enable (s);
	_audio_stream->Enable (s);
	_dcp_content_type->Enable (s);
	_dcp_frame_rate->Enable (s);
	_trim_start->Enable (s);
	_trim_end->Enable (s);
	_trim_type->Enable (s);
	_dcp_ab->Enable (s);
	_colour_lut->Enable (s);
	_j2k_bandwidth->Enable (s);
	_audio_gain->Enable (s);
	_audio_gain_calculate_button->Enable (s);
	_show_audio->Enable (s);
	_audio_delay->Enable (s);
	_still_duration->Enable (s);

	setup_subtitle_control_sensitivity ();
	setup_audio_control_sensitivity ();
	setup_show_audio_sensitivity ();
}

/** Called when the `Edit filters' button has been clicked */
void
FilmEditor::edit_filters_clicked (wxCommandEvent &)
{
	FilterDialog* d = new FilterDialog (this, _film->filters());
	d->ActiveChanged.connect (bind (&Film::set_filters, _film, _1));
	d->ShowModal ();
	d->Destroy ();
}

/** Called when the scaler widget has been changed */
void
FilmEditor::scaler_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	int const n = _scaler->GetSelection ();
	if (n >= 0) {
		_film->set_scaler (Scaler::from_index (n));
	}
}

void
FilmEditor::audio_gain_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_audio_gain (_audio_gain->GetValue ());
}

void
FilmEditor::audio_delay_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_audio_delay (_audio_delay->GetValue ());
}

wxControl *
FilmEditor::video_control (wxControl* c)
{
	_video_controls.push_back (c);
	return c;
}

wxControl *
FilmEditor::still_control (wxControl* c)
{
	_still_controls.push_back (c);
	return c;
}

void
FilmEditor::setup_visibility ()
{
	ContentType c = VIDEO;

	if (_film) {
		c = _film->content_type ();
	}

	for (list<wxControl*>::iterator i = _video_controls.begin(); i != _video_controls.end(); ++i) {
		(*i)->Show (c == VIDEO);
	}

	for (list<wxControl*>::iterator i = _still_controls.begin(); i != _still_controls.end(); ++i) {
		(*i)->Show (c == STILL);
	}

	setup_notebook_size ();
}

void
FilmEditor::setup_notebook_size ()
{
	_notebook->InvalidateBestSize ();
	
	_film_sizer->Layout ();
	_film_sizer->SetSizeHints (_film_panel);
	_video_sizer->Layout ();
	_video_sizer->SetSizeHints (_video_panel);
	_audio_sizer->Layout ();
	_audio_sizer->SetSizeHints (_audio_panel);
	_subtitle_sizer->Layout ();
	_subtitle_sizer->SetSizeHints (_subtitle_panel);

	_notebook->Fit ();
	Fit ();
}

void
FilmEditor::still_duration_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_still_duration (_still_duration->GetValue ());
}

void
FilmEditor::trim_start_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_trim_start (_trim_start->GetValue ());
}

void
FilmEditor::trim_end_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_trim_end (_trim_end->GetValue ());
}

void
FilmEditor::audio_gain_calculate_button_clicked (wxCommandEvent &)
{
	GainCalculatorDialog* d = new GainCalculatorDialog (this);
	d->ShowModal ();

	if (d->wanted_fader() == 0 || d->actual_fader() == 0) {
		d->Destroy ();
		return;
	}
	
	_audio_gain->SetValue (
		Config::instance()->sound_processor()->db_for_fader_change (
			d->wanted_fader (),
			d->actual_fader ()
			)
		);

	/* This appears to be necessary, as the change is not signalled,
	   I think.
	*/
	wxCommandEvent dummy;
	audio_gain_changed (dummy);
	
	d->Destroy ();
}

void
FilmEditor::setup_formats ()
{
	ContentType c = VIDEO;

	if (_film) {
		c = _film->content_type ();
	}
	
	_formats.clear ();

	vector<Format const *> fmt = Format::all ();
	for (vector<Format const *>::iterator i = fmt.begin(); i != fmt.end(); ++i) {
		if (c == VIDEO || (c == STILL && dynamic_cast<VariableFormat const *> (*i))) {
			_formats.push_back (*i);
		}
	}

	_format->Clear ();
	for (vector<Format const *>::iterator i = _formats.begin(); i != _formats.end(); ++i) {
		_format->Append (std_to_wx ((*i)->name ()));
	}

	_film_sizer->Layout ();
}

void
FilmEditor::with_subtitles_toggled (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_with_subtitles (_with_subtitles->GetValue ());
}

void
FilmEditor::setup_subtitle_control_sensitivity ()
{
	bool h = false;
	if (_generally_sensitive && _film) {
		h = !_film->subtitle_streams().empty();
	}
	
	_with_subtitles->Enable (h);

	bool j = false;
	if (_film) {
		j = _film->with_subtitles ();
	}
	
	_subtitle_stream->Enable (j);
	_subtitle_offset->Enable (j);
	_subtitle_scale->Enable (j);
}

void
FilmEditor::setup_audio_control_sensitivity ()
{
	_use_content_audio->Enable (_generally_sensitive && _film && !_film->content_audio_streams().empty());
	_use_external_audio->Enable (_generally_sensitive);
	
	bool const source = _generally_sensitive && _use_content_audio->GetValue();
	bool const external = _generally_sensitive && _use_external_audio->GetValue();

	_audio_stream->Enable (source);
	for (int i = 0; i < MAX_AUDIO_CHANNELS; ++i) {
		_external_audio[i]->Enable (external);
	}

	_pad_with_silence->Enable (_generally_sensitive && _film && _film->audio_stream() && _film->audio_stream()->channels() < MAX_AUDIO_CHANNELS);
	_minimum_audio_channels->Enable (_generally_sensitive && _pad_with_silence->GetValue ());
}

void
FilmEditor::use_dci_name_toggled (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_use_dci_name (_use_dci_name->GetValue ());
}

void
FilmEditor::edit_dci_button_clicked (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	DCIMetadataDialog* d = new DCIMetadataDialog (this, _film->dci_metadata ());
	d->ShowModal ();
	_film->set_dci_metadata (d->dci_metadata ());
	d->Destroy ();
}

void
FilmEditor::setup_streams ()
{
	_audio_stream->Clear ();
	vector<shared_ptr<AudioStream> > a = _film->content_audio_streams ();
	for (vector<shared_ptr<AudioStream> >::iterator i = a.begin(); i != a.end(); ++i) {
		shared_ptr<FFmpegAudioStream> ffa = dynamic_pointer_cast<FFmpegAudioStream> (*i);
		assert (ffa);
		_audio_stream->Append (std_to_wx (ffa->name()), new wxStringClientData (std_to_wx (ffa->to_string ())));
	}
	
	if (_film->use_content_audio() && _film->audio_stream()) {
		checked_set (_audio_stream, _film->audio_stream()->to_string());
	}

	_subtitle_stream->Clear ();
	vector<shared_ptr<SubtitleStream> > s = _film->subtitle_streams ();
	for (vector<shared_ptr<SubtitleStream> >::iterator i = s.begin(); i != s.end(); ++i) {
		_subtitle_stream->Append (std_to_wx ((*i)->name()), new wxStringClientData (std_to_wx ((*i)->to_string ())));
	}
	if (_film->subtitle_stream()) {
		checked_set (_subtitle_stream, _film->subtitle_stream()->to_string());
	} else {
		_subtitle_stream->SetSelection (wxNOT_FOUND);
	}
}

void
FilmEditor::audio_stream_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_content_audio_stream (
		audio_stream_factory (
			string_client_data (_audio_stream->GetClientObject (_audio_stream->GetSelection ())),
			Film::state_version
			)
		);
}

void
FilmEditor::subtitle_stream_changed (wxCommandEvent &)
{
	if (!_film) {
		return;
	}

	_film->set_subtitle_stream (
		subtitle_stream_factory (
			string_client_data (_subtitle_stream->GetClientObject (_subtitle_stream->GetSelection ())),
			Film::state_version
			)
		);
}

void
FilmEditor::setup_audio_details ()
{
	if (!_film->content_audio_stream()) {
		_audio->SetLabel (wxT (""));
	} else {
		wxString s;
		if (_film->audio_stream()->channels() == 1) {
			s << _("1 channel");
		} else {
			s << _film->audio_stream()->channels () << wxT (" ") << _("channels");
		}
		s << wxT (", ") << _film->audio_stream()->sample_rate() << _("Hz");
		_audio->SetLabel (s);
	}

	setup_notebook_size ();
}

void
FilmEditor::active_jobs_changed (bool a)
{
	set_things_sensitive (!a);
}

void
FilmEditor::use_audio_changed (wxCommandEvent &)
{
	_film->set_use_content_audio (_use_content_audio->GetValue());
}

void
FilmEditor::external_audio_changed (wxCommandEvent &)
{
	vector<string> a;
	for (int i = 0; i < MAX_AUDIO_CHANNELS; ++i) {
		a.push_back (wx_to_std (_external_audio[i]->GetPath()));
	}

	_film->set_external_audio (a);
}

void
FilmEditor::setup_dcp_name ()
{
	string s = _film->dcp_name (true);
	if (s.length() > 28) {
		_dcp_name->SetLabel (std_to_wx (s.substr (0, 28)) + N_("..."));
		_dcp_name->SetToolTip (std_to_wx (s));
	} else {
		_dcp_name->SetLabel (std_to_wx (s));
	}
}

void
FilmEditor::show_audio_clicked (wxCommandEvent &)
{
	if (_audio_dialog) {
		_audio_dialog->Destroy ();
		_audio_dialog = 0;
	}
	
	_audio_dialog = new AudioDialog (this);
	_audio_dialog->Show ();
	_audio_dialog->set_film (_film);
}

void
FilmEditor::best_dcp_frame_rate_clicked (wxCommandEvent &)
{
	if (!_film) {
		return;
	}
	
	_film->set_dcp_frame_rate (best_dcp_frame_rate (_film->source_frame_rate ()));
}

void
FilmEditor::setup_show_audio_sensitivity ()
{
	_show_audio->Enable (_film && _film->has_audio ());
}

void
FilmEditor::setup_scaling_description ()
{
	wxString d;

	int lines = 0;

	if (_film->size().width && _film->size().height) {
		d << wxString::Format (
			_("Original video is %dx%d (%.2f:1)\n"),
			_film->size().width, _film->size().height,
			float (_film->size().width) / _film->size().height
			);
		++lines;
	}

	Crop const crop = _film->crop ();
	if ((crop.left || crop.right || crop.top || crop.bottom) && _film->size() != libdcp::Size (0, 0)) {
		libdcp::Size const cropped = _film->cropped_size (_film->size ());
		d << wxString::Format (
			_("Cropped to %dx%d (%.2f:1)\n"),
			cropped.width, cropped.height,
			float (cropped.width) / cropped.height
			);
		++lines;
	}

	Format const * format = _film->format ();
	if (format) {
		int const padding = format->dcp_padding (_film);
		libdcp::Size scaled = format->dcp_size ();
		scaled.width -= padding * 2;
		d << wxString::Format (
			_("Scaled to %dx%d (%.2f:1)\n"),
			scaled.width, scaled.height,
			float (scaled.width) / scaled.height
			);
		++lines;

		if (padding) {
			d << wxString::Format (
				_("Padded with black to %dx%d (%.2f:1)\n"),
				format->dcp_size().width, format->dcp_size().height,
				float (format->dcp_size().width) / format->dcp_size().height
				);
			++lines;
		}
	}

	for (int i = lines; i < 4; ++i) {
		d << wxT ("\n ");
	}

	_scaling_description->SetLabel (d);
}

void
FilmEditor::trim_type_changed (wxCommandEvent &)
{
	_film->set_trim_type (_trim_type->GetSelection () == 0 ? Film::CPL : Film::ENCODE);
}

void
FilmEditor::setup_minimum_audio_channels ()
{
	if (!_film || !_film->audio_stream ()) {
		_pad_with_silence->SetValue (false);
		return;
	}

	_pad_with_silence->SetValue (_film->audio_stream()->channels() < _film->minimum_audio_channels());

	AudioMapping m (_film);
	/* For some bizarre reason this call results in minimum_audio_channels_changed being
	   called with _minimum_audio_channels' value set to the low end of the range
	   (on Windows only)
	*/
	_ignore_minimum_audio_channels_change = true;
	_minimum_audio_channels->SetRange (m.minimum_dcp_channels() + 1, MAX_AUDIO_CHANNELS);
	_ignore_minimum_audio_channels_change = false;
}

void
FilmEditor::pad_with_silence_toggled (wxCommandEvent &)
{
	setup_audio_control_sensitivity ();

	if (!_pad_with_silence->GetValue ()) {
		_film->set_minimum_audio_channels (0);
	}
}

void
FilmEditor::minimum_audio_channels_changed (wxCommandEvent &)
{
	if (!_film || _ignore_minimum_audio_channels_change) {
		return;
	}

	if (_pad_with_silence->GetValue ()) {
		_film->set_minimum_audio_channels (_minimum_audio_channels->GetValue ());
	}
}

void
FilmEditor::setup_warnings ()
{
	string w;

	int c = max (_film->audio_channels (), _film->minimum_audio_channels ());
	
	if (c == 0) {
		w += _("The film has no audio.  This will cause problems on some projectors.  "
		       "Try setting 'pad with silence' in the Audio tab to 6 channels.\n");
	} else if (c % 2) {
		w += _("The film has an odd number of audio channels.  This will cause problems on some projectors.  "
		       "Try setting 'pad with silence' in the Audio tab to 6 channels.\n");
	} else if (c < 6) {
		w += _("The film has fewer than 6 audio channels.  This will cause problems on a few projectors.  "
		       "Try setting 'pad with silence' in the Audio tab to 6 channels.\n");
	}

	if (_film->dcp_frame_rate() != 24) {
		w += _("This film is not at 24 frames-per-second.  Some older projectors will not play it.\n");
	}

	checked_set (_warnings, w);
}
