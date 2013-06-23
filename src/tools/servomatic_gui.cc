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

#include <boost/thread.hpp>
#include <wx/taskbar.h>
#include <wx/icon.h>
#include "wx_util.h"
#include "lib/util.h"
#include "lib/server.h"
#include "lib/config.h"

using std::cout;
using std::string;
using boost::shared_ptr;
using boost::thread;
using boost::bind;

enum {
	ID_status = 1,
	ID_quit,
	ID_timer
};

class MemoryLog : public Log
{
public:

	string get () const {
		boost::mutex::scoped_lock (_mutex);
		return _log;
	}

private:
	void do_log (string m)
	{
		_log = m;
	}

	string _log;	
};

static shared_ptr<MemoryLog> memory_log (new MemoryLog);

class StatusDialog : public wxDialog
{
public:
	StatusDialog ()
		: wxDialog (0, wxID_ANY, _("DVD-o-matic encode server"), wxDefaultPosition, wxSize (600, 80), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
		, _timer (this, ID_timer)
	{
		_sizer = new wxFlexGridSizer (1, 6, 6);
		_sizer->AddGrowableCol (0, 1);

		_text = new wxTextCtrl (this, wxID_ANY, _(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
		_sizer->Add (_text, 1, wxEXPAND);

		SetSizer (_sizer);
		_sizer->Layout ();

		Connect (ID_timer, wxEVT_TIMER, wxTimerEventHandler (StatusDialog::update));
		_timer.Start (1000);
	}

private:
	void update (wxTimerEvent &)
	{
		_text->ChangeValue (std_to_wx (memory_log->get ()));
		_sizer->Layout ();
	}

	wxFlexGridSizer* _sizer;
	wxTextCtrl* _text;
	wxTimer _timer;
};

class TaskBarIcon : public wxTaskBarIcon
{
public:
	TaskBarIcon ()
	{
#ifdef __WXMSW__		
		wxIcon icon (std_to_wx ("taskbar_icon"));
#endif
#ifdef __WXGTK__
		wxInitAllImageHandlers();
		wxBitmap bitmap (wxString::Format (wxT ("%s/taskbar_icon.png"), POSIX_ICON_PREFIX), wxBITMAP_TYPE_PNG);
		wxIcon icon;
		icon.CopyFromBitmap (bitmap);
#endif
#ifndef __WXOSX__
		/* XXX: fix this for OS X */
		SetIcon (icon, std_to_wx ("DVD-o-matic encode server"));
#endif		

		Connect (ID_status, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler (TaskBarIcon::status));
		Connect (ID_quit, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler (TaskBarIcon::quit));
	}
	
	wxMenu* CreatePopupMenu ()
	{
		wxMenu* menu = new wxMenu;
		menu->Append (ID_status, std_to_wx ("Status..."));
		menu->Append (ID_quit, std_to_wx ("Quit"));
		return menu;
	}

private:
	void status (wxCommandEvent &)
	{
		StatusDialog* d = new StatusDialog;
		d->Show ();
	}

	void quit (wxCommandEvent &)
	{
		wxTheApp->ExitMainLoop ();
	}
};

class App : public wxApp
{
public:
	App ()
		: wxApp ()
		, _thread (0)
		, _icon (0)
	{}

private:	
	
	bool OnInit ()
	{
		if (!wxApp::OnInit ()) {
			return false;
		}
		
		dvdomatic_setup ();

		_icon = new TaskBarIcon;
		_thread = new thread (bind (&App::main_thread, this));
		
		return true;
	}

	int OnExit ()
	{
		delete _icon;
		return wxApp::OnExit ();
	}

	void main_thread ()
	{
		Server server (memory_log);
		server.run (Config::instance()->num_local_encoding_threads ());
	}

	boost::thread* _thread;
	TaskBarIcon* _icon;
};

IMPLEMENT_APP (App)