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

#include <libdcp/raw_convert.h>
#include "lib/content.h"
#include "lib/image_content.h"
#include "timing_panel.h"
#include "wx_util.h"
#include "timecode.h"
#include "film_editor.h"

using std::cout;
using std::string;
using std::set;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using libdcp::raw_convert;

TimingPanel::TimingPanel (FilmEditor* e)
	/* horrid hack for apparent lack of context support with wxWidgets i18n code */
	: FilmEditorPanel (e, S_("Timing|Timing"))
{
	wxFlexGridSizer* grid = new wxFlexGridSizer (2, 4, 4);
	_sizer->Add (grid, 0, wxALL, 8);

	wxSize size = Timecode::size (this);
	
	wxSizer* labels = new wxBoxSizer (wxHORIZONTAL);
	//// TRANSLATORS: this is an abbreviation for "hours"
	wxStaticText* t = new wxStaticText (this, wxID_ANY, _("h"), wxDefaultPosition, size, wxALIGN_CENTRE_HORIZONTAL);
#ifdef DCPOMATIC_LINUX
	/* Hack to work around failure to centre text on GTK */
	gtk_label_set_line_wrap (GTK_LABEL (t->GetHandle()), FALSE);
#endif		
	labels->Add (t, 1, wxEXPAND);
	add_label_to_sizer (labels, this, wxT (":"), false);
	//// TRANSLATORS: this is an abbreviation for "minutes"
	t = new wxStaticText (this, wxID_ANY, _("m"), wxDefaultPosition, size, wxALIGN_CENTRE_HORIZONTAL);
#ifdef DCPOMATIC_LINUX
	gtk_label_set_line_wrap (GTK_LABEL (t->GetHandle()), FALSE);
#endif		
	labels->Add (t, 1, wxEXPAND);
	add_label_to_sizer (labels, this, wxT (":"), false);
	//// TRANSLATORS: this is an abbreviation for "seconds"
	t = new wxStaticText (this, wxID_ANY, _("s"), wxDefaultPosition, size, wxALIGN_CENTRE_HORIZONTAL);
#ifdef DCPOMATIC_LINUX
	gtk_label_set_line_wrap (GTK_LABEL (t->GetHandle()), FALSE);
#endif		
	labels->Add (t, 1, wxEXPAND);
	add_label_to_sizer (labels, this, wxT (":"), false);
	//// TRANSLATORS: this is an abbreviation for "frames"
	t = new wxStaticText (this, wxID_ANY, _("f"), wxDefaultPosition, size, wxALIGN_CENTRE_HORIZONTAL);
#ifdef DCPOMATIC_LINUX
	gtk_label_set_line_wrap (GTK_LABEL (t->GetHandle()), FALSE);
#endif		
	labels->Add (t, 1, wxEXPAND);
	grid->Add (new wxStaticText (this, wxID_ANY, wxT ("")));
	grid->Add (labels);

	add_label_to_sizer (grid, this, _("Position"), true);
	_position = new Timecode (this);
	grid->Add (_position);
	add_label_to_sizer (grid, this, _("Full length"), true);
	_full_length = new Timecode (this);
	grid->Add (_full_length);
	add_label_to_sizer (grid, this, _("Trim from start"), true);
	_trim_start = new Timecode (this);
	grid->Add (_trim_start);
	add_label_to_sizer (grid, this, _("Trim from end"), true);
	_trim_end = new Timecode (this);
	grid->Add (_trim_end);
	add_label_to_sizer (grid, this, _("Play length"), true);
	_play_length = new Timecode (this);
	grid->Add (_play_length);

	{
		add_label_to_sizer (grid, this, _("Video frame rate"), true);
		wxBoxSizer* s = new wxBoxSizer (wxHORIZONTAL);
		_video_frame_rate = new wxTextCtrl (this, wxID_ANY);
		s->Add (_video_frame_rate, 1, wxEXPAND);
		_set_video_frame_rate = new wxButton (this, wxID_ANY, _("Set"));
		_set_video_frame_rate->Enable (false);
		s->Add (_set_video_frame_rate, 0, wxLEFT | wxRIGHT, 8);
		grid->Add (s, 1, wxEXPAND);
	}

	_position->Changed.connect    (boost::bind (&TimingPanel::position_changed, this));
	_full_length->Changed.connect (boost::bind (&TimingPanel::full_length_changed, this));
	_trim_start->Changed.connect  (boost::bind (&TimingPanel::trim_start_changed, this));
	_trim_end->Changed.connect    (boost::bind (&TimingPanel::trim_end_changed, this));
	_play_length->Changed.connect (boost::bind (&TimingPanel::play_length_changed, this));
	_video_frame_rate->Bind       (wxEVT_COMMAND_TEXT_UPDATED, boost::bind (&TimingPanel::video_frame_rate_changed, this));
	_set_video_frame_rate->Bind   (wxEVT_COMMAND_BUTTON_CLICKED, boost::bind (&TimingPanel::set_video_frame_rate, this));
}

void
TimingPanel::film_content_changed (int property)
{
	ContentList cl = _editor->selected_content ();
	int const film_video_frame_rate = _editor->film()->video_frame_rate ();

	/* Here we check to see if we have exactly one different value of various
	   properties, and fill the controls with that value if so.
	*/
	
	if (property == ContentProperty::POSITION) {

		set<Time> check;
		for (ContentList::const_iterator i = cl.begin (); i != cl.end(); ++i) {
			check.insert ((*i)->position ());
		}

		if (check.size() == 1) {
			_position->set (cl.front()->position(), film_video_frame_rate);
		} else {
			_position->clear ();
		}
		
	} else if (
		property == ContentProperty::LENGTH ||
		property == VideoContentProperty::VIDEO_FRAME_RATE ||
		property == VideoContentProperty::VIDEO_FRAME_TYPE
		) {

		set<Time> check;
		for (ContentList::const_iterator i = cl.begin (); i != cl.end(); ++i) {
			check.insert ((*i)->full_length ());
		}
		
		if (check.size() == 1) {
			_full_length->set (cl.front()->full_length (), film_video_frame_rate);
		} else {
			_full_length->clear ();
		}

	} else if (property == ContentProperty::TRIM_START) {

		set<Time> check;
		for (ContentList::const_iterator i = cl.begin (); i != cl.end(); ++i) {
			check.insert ((*i)->trim_start ());
		}
		
		if (check.size() == 1) {
			_trim_start->set (cl.front()->trim_start (), film_video_frame_rate);
		} else {
			_trim_start->clear ();
		}
		
	} else if (property == ContentProperty::TRIM_END) {

		set<Time> check;
		for (ContentList::const_iterator i = cl.begin (); i != cl.end(); ++i) {
			check.insert ((*i)->trim_end ());
		}
		
		if (check.size() == 1) {
			_trim_end->set (cl.front()->trim_end (), film_video_frame_rate);
		} else {
			_trim_end->set (0, 24);
		}
	}

	if (
		property == ContentProperty::LENGTH ||
		property == ContentProperty::TRIM_START ||
		property == ContentProperty::TRIM_END ||
		property == VideoContentProperty::VIDEO_FRAME_RATE ||
		property == VideoContentProperty::VIDEO_FRAME_TYPE
		) {

		set<Time> check;
		for (ContentList::const_iterator i = cl.begin (); i != cl.end(); ++i) {
			check.insert ((*i)->length_after_trim ());
		}
		
		if (check.size() == 1) {
			_play_length->set (cl.front()->length_after_trim (), film_video_frame_rate);
		} else {
			_play_length->clear ();
		}
	}

	if (property == VideoContentProperty::VIDEO_FRAME_RATE) {
		set<float> check;
		shared_ptr<VideoContent> vc;
		for (ContentList::const_iterator i = cl.begin (); i != cl.end(); ++i) {
			shared_ptr<VideoContent> t = dynamic_pointer_cast<VideoContent> (*i);
			if (t) {
				check.insert (t->video_frame_rate ());
				vc = t;
			}
		}
		if (check.size() == 1) {
			_video_frame_rate->SetValue (std_to_wx (raw_convert<string> (vc->video_frame_rate (), 5)));
			_video_frame_rate->Enable (true);
		} else {
			_video_frame_rate->SetValue ("");
			_video_frame_rate->Enable (false);
		}
	}

	bool have_still = false;
	for (ContentList::const_iterator i = cl.begin (); i != cl.end(); ++i) {
		shared_ptr<ImageContent> ic = dynamic_pointer_cast<ImageContent> (*i);
		if (ic && ic->still ()) {
			have_still = true;
		}
	}

	_full_length->set_editable (have_still);
	_play_length->set_editable (!have_still);
	_set_video_frame_rate->Enable (false);
}

void
TimingPanel::position_changed ()
{
	ContentList c = _editor->selected_content ();
	for (ContentList::iterator i = c.begin(); i != c.end(); ++i) {
		(*i)->set_position (_position->get (_editor->film()->video_frame_rate ()));
	}
}

void
TimingPanel::full_length_changed ()
{
	ContentList c = _editor->selected_content ();
	for (ContentList::iterator i = c.begin(); i != c.end(); ++i) {
		shared_ptr<ImageContent> ic = dynamic_pointer_cast<ImageContent> (*i);
		if (ic && ic->still ()) {
			ic->set_video_length (rint (_full_length->get (_editor->film()->video_frame_rate()) * ic->video_frame_rate() / TIME_HZ));
		}
	}
}

void
TimingPanel::trim_start_changed ()
{
	ContentList c = _editor->selected_content ();
	for (ContentList::iterator i = c.begin(); i != c.end(); ++i) {
		(*i)->set_trim_start (_trim_start->get (_editor->film()->video_frame_rate ()));
	}
}


void
TimingPanel::trim_end_changed ()
{
	ContentList c = _editor->selected_content ();
	for (ContentList::iterator i = c.begin(); i != c.end(); ++i) {
		(*i)->set_trim_end (_trim_end->get (_editor->film()->video_frame_rate ()));
	}
}

void
TimingPanel::play_length_changed ()
{
	ContentList c = _editor->selected_content ();
	for (ContentList::iterator i = c.begin(); i != c.end(); ++i) {
		(*i)->set_trim_end ((*i)->full_length() - _play_length->get (_editor->film()->video_frame_rate()) - (*i)->trim_start());
	}
}

void
TimingPanel::video_frame_rate_changed ()
{
	_set_video_frame_rate->Enable (true);
}

void
TimingPanel::set_video_frame_rate ()
{
	ContentList c = _editor->selected_content ();
	for (ContentList::iterator i = c.begin(); i != c.end(); ++i) {
		shared_ptr<VideoContent> vc = dynamic_pointer_cast<VideoContent> (*i);
		if (vc) {
			vc->set_video_frame_rate (raw_convert<float> (wx_to_std (_video_frame_rate->GetValue ())));
		}
		_set_video_frame_rate->Enable (false);
	}
}

void
TimingPanel::content_selection_changed ()
{
	bool const e = !_editor->selected_content().empty ();

	_position->Enable (e);
	_full_length->Enable (e);
	_trim_start->Enable (e);
	_trim_end->Enable (e);
	_play_length->Enable (e);
	_video_frame_rate->Enable (e);
	
	film_content_changed (ContentProperty::POSITION);
	film_content_changed (ContentProperty::LENGTH);
	film_content_changed (ContentProperty::TRIM_START);
	film_content_changed (ContentProperty::TRIM_END);
	film_content_changed (VideoContentProperty::VIDEO_FRAME_RATE);
}
