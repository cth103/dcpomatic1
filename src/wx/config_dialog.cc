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

/** @file src/config_dialog.cc
 *  @brief A dialogue to edit DCP-o-matic configuration.
 */

#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <wx/stdpaths.h>
#include <wx/preferences.h>
#include <wx/filepicker.h>
#include <wx/spinctrl.h>
#include <libdcp/colour_matrix.h>
#include "lib/config.h"
#include "lib/ratio.h"
#include "lib/scaler.h"
#include "lib/filter.h"
#include "lib/dcp_content_type.h"
#include "lib/colour_conversion.h"
#include "config_dialog.h"
#include "wx_util.h"
#include "editable_list.h"
#include "filter_dialog.h"
#include "dir_picker_ctrl.h"
#include "isdcf_metadata_dialog.h"
#include "preset_colour_conversion_dialog.h"
#include "server_dialog.h"

using std::vector;
using std::string;
using std::list;
using std::cout;
using boost::bind;
using boost::shared_ptr;
using boost::lexical_cast;

class Page
{
public:
	Page (wxSize panel_size, int border)
		: _panel_size (panel_size)
		, _border (border)
	{}

protected:
	wxPanel* make_panel (wxWindow* parent)
	{
		wxPanel* panel = new wxPanel (parent, wxID_ANY, wxDefaultPosition, _panel_size);
		wxBoxSizer* s = new wxBoxSizer (wxVERTICAL);
		panel->SetSizer (s);
		return panel;
	}
	
	wxSize _panel_size;
	int _border;
};

class GeneralPage : public wxStockPreferencesPage, public Page
{
public:
	GeneralPage (wxSize panel_size, int border)
		: wxStockPreferencesPage (Kind_General)
		, Page (panel_size, border)
	{}

	wxWindow* CreateWindow (wxWindow* parent)
	{
		wxPanel* panel = make_panel (parent);

		wxFlexGridSizer* table = new wxFlexGridSizer (2, DCPOMATIC_SIZER_X_GAP, DCPOMATIC_SIZER_Y_GAP);
		table->AddGrowableCol (1, 1);
		panel->GetSizer()->Add (table, 1, wxALL | wxEXPAND, _border);
		
		_set_language = new wxCheckBox (panel, wxID_ANY, _("Set language"));
		table->Add (_set_language, 1);
		_language = new wxChoice (panel, wxID_ANY);
		_language->Append (wxT ("Deutsch"));
		_language->Append (wxT ("English"));
		_language->Append (wxT ("Español"));
		_language->Append (wxT ("Français"));
		_language->Append (wxT ("Italiano"));
		_language->Append (wxT ("Nederlands"));
		_language->Append (wxT ("Svenska"));
		table->Add (_language);
		
		wxStaticText* restart = add_label_to_sizer (table, panel, _("(restart DCP-o-matic to see language changes)"), false);
		wxFont font = restart->GetFont();
		font.SetStyle (wxFONTSTYLE_ITALIC);
		font.SetPointSize (font.GetPointSize() - 1);
		restart->SetFont (font);
		table->AddSpacer (0);
		
		add_label_to_sizer (table, panel, _("Threads to use for encoding on this host"), true);
		_num_local_encoding_threads = new wxSpinCtrl (panel);
		table->Add (_num_local_encoding_threads, 1);

		
		_check_for_updates = new wxCheckBox (panel, wxID_ANY, _("Check for updates on startup"));
		table->Add (_check_for_updates, 1, wxEXPAND | wxALL);
		table->AddSpacer (0);
		
		_check_for_test_updates = new wxCheckBox (panel, wxID_ANY, _("Check for testing updates as well as stable ones"));
		table->Add (_check_for_test_updates, 1, wxEXPAND | wxALL);
		table->AddSpacer (0);
		
		Config* config = Config::instance ();
		
		_set_language->SetValue (config->language ());
		
		if (config->language().get_value_or ("") == "fr") {
			_language->SetSelection (3);
		} else if (config->language().get_value_or ("") == "it") {
			_language->SetSelection (4);
		} else if (config->language().get_value_or ("") == "es") {
			_language->SetSelection (2);
		} else if (config->language().get_value_or ("") == "sv") {
			_language->SetSelection (6);
		} else if (config->language().get_value_or ("") == "de") {
			_language->SetSelection (0);
		} else if (config->language().get_value_or ("") == "nl") {
			_language->SetSelection (5);
		} else {
			_language->SetSelection (1);
		}
		
		setup_language_sensitivity ();
		
		_set_language->Bind (wxEVT_COMMAND_CHECKBOX_CLICKED, boost::bind (&GeneralPage::set_language_changed, this));
		_language->Bind     (wxEVT_COMMAND_CHOICE_SELECTED,  boost::bind (&GeneralPage::language_changed,     this));
		
		_num_local_encoding_threads->SetRange (1, 128);
		_num_local_encoding_threads->SetValue (config->num_local_encoding_threads ());
		_num_local_encoding_threads->Bind (wxEVT_COMMAND_SPINCTRL_UPDATED, boost::bind (&GeneralPage::num_local_encoding_threads_changed, this));

		_check_for_updates->SetValue (config->check_for_updates ());
		_check_for_updates->Bind (wxEVT_COMMAND_CHECKBOX_CLICKED, boost::bind (&GeneralPage::check_for_updates_changed, this));
		_check_for_test_updates->SetValue (config->check_for_test_updates ());
		_check_for_test_updates->Bind (wxEVT_COMMAND_CHECKBOX_CLICKED, boost::bind (&GeneralPage::check_for_test_updates_changed, this));
		
		return panel;
	}

private:	
	void setup_language_sensitivity ()
	{
		_language->Enable (_set_language->GetValue ());
	}

	void set_language_changed ()
	{
		setup_language_sensitivity ();
		if (_set_language->GetValue ()) {
			language_changed ();
		} else {
			Config::instance()->unset_language ();
		}
	}

	void language_changed ()
	{
		switch (_language->GetSelection ()) {
		case 0:
			Config::instance()->set_language ("de");
			break;
		case 1:
			Config::instance()->set_language ("en");
			break;
		case 2:
			Config::instance()->set_language ("es");
			break;
		case 3:
			Config::instance()->set_language ("fr");
			break;
		case 4:
			Config::instance()->set_language ("it");
			break;
		case 5:
			Config::instance()->set_language ("nl");
			break;
		case 6:
			Config::instance()->set_language ("sv");
			break;
		}
	}
	
	void check_for_updates_changed ()
	{
		Config::instance()->set_check_for_updates (_check_for_updates->GetValue ());
	}
	
	void check_for_test_updates_changed ()
	{
		Config::instance()->set_check_for_test_updates (_check_for_test_updates->GetValue ());
	}

	void num_local_encoding_threads_changed ()
	{
		Config::instance()->set_num_local_encoding_threads (_num_local_encoding_threads->GetValue ());
	}

	wxCheckBox* _set_language;
	wxChoice* _language;
	wxSpinCtrl* _num_local_encoding_threads;
	wxCheckBox* _check_for_updates;
	wxCheckBox* _check_for_test_updates;
};

class DefaultsPage : public wxPreferencesPage, public Page
{
public:
	DefaultsPage (wxSize panel_size, int border)
		: Page (panel_size, border)
	{}
	
	wxString GetName () const
	{
		return _("Defaults");
	}

#ifdef DCPOMATIC_OSX	
	wxBitmap GetLargeIcon () const
	{
		return wxBitmap ("defaults", wxBITMAP_TYPE_PNG_RESOURCE);
	}
#endif	

	wxWindow* CreateWindow (wxWindow* parent)
	{
		wxPanel* panel = make_panel (parent);

		wxFlexGridSizer* table = new wxFlexGridSizer (2, DCPOMATIC_SIZER_X_GAP, DCPOMATIC_SIZER_Y_GAP);
		table->AddGrowableCol (1, 1);
		panel->GetSizer()->Add (table, 1, wxALL | wxEXPAND, _border);
		
		{
			add_label_to_sizer (table, panel, _("Default duration of still images"), true);
			wxBoxSizer* s = new wxBoxSizer (wxHORIZONTAL);
			_still_length = new wxSpinCtrl (panel);
			s->Add (_still_length);
			add_label_to_sizer (s, panel, _("s"), false);
			table->Add (s, 1);
		}
		
		add_label_to_sizer (table, panel, _("Default directory for new films"), true);
#ifdef DCPOMATIC_USE_OWN_DIR_PICKER
		_directory = new DirPickerCtrl (panel);
#else	
		_directory = new wxDirPickerCtrl (panel, wxDD_DIR_MUST_EXIST);
#endif
		table->Add (_directory, 1, wxEXPAND);
		
		add_label_to_sizer (table, panel, _("Default ISDCF name details"), true);
		_isdcf_metadata_button = new wxButton (panel, wxID_ANY, _("Edit..."));
		table->Add (_isdcf_metadata_button);

		add_label_to_sizer (table, panel, _("Default container"), true);
		_container = new wxChoice (panel, wxID_ANY);
		table->Add (_container);
		
		add_label_to_sizer (table, panel, _("Default content type"), true);
		_dcp_content_type = new wxChoice (panel, wxID_ANY);
		table->Add (_dcp_content_type);
		
		{
			add_label_to_sizer (table, panel, _("Default JPEG2000 bandwidth"), true);
			wxBoxSizer* s = new wxBoxSizer (wxHORIZONTAL);
			_j2k_bandwidth = new wxSpinCtrl (panel);
			s->Add (_j2k_bandwidth);
			add_label_to_sizer (s, panel, _("Mbit/s"), false);
			table->Add (s, 1);
		}
		
		{
			add_label_to_sizer (table, panel, _("Default audio delay"), true);
			wxBoxSizer* s = new wxBoxSizer (wxHORIZONTAL);
			_audio_delay = new wxSpinCtrl (panel);
			s->Add (_audio_delay);
			add_label_to_sizer (s, panel, _("ms"), false);
			table->Add (s, 1);
		}

		add_label_to_sizer (table, panel, _("Default issuer"), true);
		_issuer = new wxTextCtrl (panel, wxID_ANY);
		table->Add (_issuer, 1, wxEXPAND);

		Config* config = Config::instance ();
		
		_still_length->SetRange (1, 3600);
		_still_length->SetValue (config->default_still_length ());
		_still_length->Bind (wxEVT_COMMAND_SPINCTRL_UPDATED, boost::bind (&DefaultsPage::still_length_changed, this));
		
		_directory->SetPath (std_to_wx (config->default_directory_or (wx_to_std (wxStandardPaths::Get().GetDocumentsDir())).string ()));
		_directory->Bind (wxEVT_COMMAND_DIRPICKER_CHANGED, boost::bind (&DefaultsPage::directory_changed, this));
		
		_isdcf_metadata_button->Bind (wxEVT_COMMAND_BUTTON_CLICKED, boost::bind (&DefaultsPage::edit_isdcf_metadata_clicked, this, parent));
		
		vector<Ratio const *> ratios = Ratio::all ();
		for (size_t i = 0; i < ratios.size(); ++i) {
			_container->Append (std_to_wx (ratios[i]->nickname ()));
			if (ratios[i] == config->default_container ()) {
				_container->SetSelection (i);
			}
		}
		
		_container->Bind (wxEVT_COMMAND_CHOICE_SELECTED, boost::bind (&DefaultsPage::container_changed, this));
		
		vector<DCPContentType const *> const ct = DCPContentType::all ();
		for (size_t i = 0; i < ct.size(); ++i) {
			_dcp_content_type->Append (std_to_wx (ct[i]->pretty_name ()));
			if (ct[i] == config->default_dcp_content_type ()) {
				_dcp_content_type->SetSelection (i);
			}
		}
		
		_dcp_content_type->Bind (wxEVT_COMMAND_CHOICE_SELECTED, boost::bind (&DefaultsPage::dcp_content_type_changed, this));
		
		_j2k_bandwidth->SetRange (50, 250);
		_j2k_bandwidth->SetValue (config->default_j2k_bandwidth() / 1000000);
		_j2k_bandwidth->Bind (wxEVT_COMMAND_SPINCTRL_UPDATED, boost::bind (&DefaultsPage::j2k_bandwidth_changed, this));
		
		_audio_delay->SetRange (-1000, 1000);
		_audio_delay->SetValue (config->default_audio_delay ());
		_audio_delay->Bind (wxEVT_COMMAND_SPINCTRL_UPDATED, boost::bind (&DefaultsPage::audio_delay_changed, this));

		_issuer->SetValue (std_to_wx (config->dcp_issuer ()));
		_issuer->Bind (wxEVT_COMMAND_TEXT_UPDATED, boost::bind (&DefaultsPage::issuer_changed, this));

		return panel;
	}

private:
	void j2k_bandwidth_changed ()
	{
		Config::instance()->set_default_j2k_bandwidth (_j2k_bandwidth->GetValue() * 1000000);
	}
	
	void audio_delay_changed ()
	{
		Config::instance()->set_default_audio_delay (_audio_delay->GetValue());
	}

	void directory_changed ()
	{
		Config::instance()->set_default_directory (wx_to_std (_directory->GetPath ()));
	}

	void edit_isdcf_metadata_clicked (wxWindow* parent)
	{
		ISDCFMetadataDialog* d = new ISDCFMetadataDialog (parent, Config::instance()->default_isdcf_metadata ());
		d->ShowModal ();
		Config::instance()->set_default_isdcf_metadata (d->isdcf_metadata ());
		d->Destroy ();
	}

	void still_length_changed ()
	{
		Config::instance()->set_default_still_length (_still_length->GetValue ());
	}

	void container_changed ()
	{
		vector<Ratio const *> ratio = Ratio::all ();
		Config::instance()->set_default_container (ratio[_container->GetSelection()]);
	}
	
	void dcp_content_type_changed ()
	{
		vector<DCPContentType const *> ct = DCPContentType::all ();
		Config::instance()->set_default_dcp_content_type (ct[_dcp_content_type->GetSelection()]);
	}

	void issuer_changed ()
	{
		Config::instance()->set_dcp_issuer (wx_to_std (_issuer->GetValue ()));
	}
	
	wxSpinCtrl* _j2k_bandwidth;
	wxSpinCtrl* _audio_delay;
	wxButton* _isdcf_metadata_button;
	wxSpinCtrl* _still_length;
#ifdef DCPOMATIC_USE_OWN_DIR_PICKER
	DirPickerCtrl* _directory;
#else
	wxDirPickerCtrl* _directory;
#endif
	wxChoice* _container;
	wxChoice* _dcp_content_type;
	wxTextCtrl* _issuer;
};

class EncodingServersPage : public wxPreferencesPage, public Page
{
public:
	EncodingServersPage (wxSize panel_size, int border)
		: Page (panel_size, border)
	{}
	
	wxString GetName () const
	{
		return _("Servers");
	}

#ifdef DCPOMATIC_OSX	
	wxBitmap GetLargeIcon () const
	{
		return wxBitmap ("servers", wxBITMAP_TYPE_PNG_RESOURCE);
	}
#endif	

	wxWindow* CreateWindow (wxWindow* parent)
	{
		wxPanel* panel = make_panel (parent);
		
		_use_any_servers = new wxCheckBox (panel, wxID_ANY, _("Use all servers"));
		panel->GetSizer()->Add (_use_any_servers, 0, wxALL, _border);
		
		vector<string> columns;
		columns.push_back (wx_to_std (_("IP address / host name")));
		_servers_list = new EditableList<string, ServerDialog> (
			panel,
			columns,
			boost::bind (&Config::servers, Config::instance()),
			boost::bind (&Config::set_servers, Config::instance(), _1),
			boost::bind (&EncodingServersPage::server_column, this, _1)
			);
		
		panel->GetSizer()->Add (_servers_list, 1, wxEXPAND | wxALL, _border);
		
		_use_any_servers->SetValue (Config::instance()->use_any_servers ());
		_use_any_servers->Bind (wxEVT_COMMAND_CHECKBOX_CLICKED, boost::bind (&EncodingServersPage::use_any_servers_changed, this));

		return panel;
	}

private:	

	void use_any_servers_changed ()
	{
		Config::instance()->set_use_any_servers (_use_any_servers->GetValue ());
	}

	string server_column (string s)
	{
		return s;
	}

	wxCheckBox* _use_any_servers;
	EditableList<string, ServerDialog>* _servers_list;
};

class ColourConversionsPage : public wxPreferencesPage, public Page
{
public:
	ColourConversionsPage (wxSize panel_size, int border)
		: Page (panel_size, border)
	{}
	
	wxString GetName () const
	{
		return _("Colour Conversions");
	}

#ifdef DCPOMATIC_OSX	
	wxBitmap GetLargeIcon () const
	{
		return wxBitmap ("colour_conversions", wxBITMAP_TYPE_PNG_RESOURCE);
	}
#endif	
	wxWindow* CreateWindow (wxWindow* parent)
	{
		wxPanel* panel = make_panel (parent);

		vector<string> columns;
		columns.push_back (wx_to_std (_("Name")));
		wxPanel* list = new EditableList<PresetColourConversion, PresetColourConversionDialog> (
			panel,
			columns,
			boost::bind (&Config::colour_conversions, Config::instance()),
			boost::bind (&Config::set_colour_conversions, Config::instance(), _1),
			boost::bind (&ColourConversionsPage::colour_conversion_column, this, _1),
			300
			);

		panel->GetSizer()->Add (list, 1, wxEXPAND | wxALL, _border);
		return panel;
	}

private:
	string colour_conversion_column (PresetColourConversion c)
	{
		return c.name;
	}
};

class TMSPage : public wxPreferencesPage, public Page
{
public:
	TMSPage (wxSize panel_size, int border)
		: Page (panel_size, border)
	{}

	wxString GetName () const
	{
		return _("TMS");
	}

#ifdef DCPOMATIC_OSX	
	wxBitmap GetLargeIcon () const
	{
		return wxBitmap ("tms", wxBITMAP_TYPE_PNG_RESOURCE);
	}
#endif	

	wxWindow* CreateWindow (wxWindow* parent)
	{
		wxPanel* panel = make_panel (parent);

		wxFlexGridSizer* table = new wxFlexGridSizer (2, DCPOMATIC_SIZER_X_GAP, DCPOMATIC_SIZER_Y_GAP);
		table->AddGrowableCol (1, 1);
		panel->GetSizer()->Add (table, 1, wxALL | wxEXPAND, _border);
		
		add_label_to_sizer (table, panel, _("IP address"), true);
		_tms_ip = new wxTextCtrl (panel, wxID_ANY);
		table->Add (_tms_ip, 1, wxEXPAND);
		
		add_label_to_sizer (table, panel, _("Target path"), true);
		_tms_path = new wxTextCtrl (panel, wxID_ANY);
		table->Add (_tms_path, 1, wxEXPAND);
		
		add_label_to_sizer (table, panel, _("User name"), true);
		_tms_user = new wxTextCtrl (panel, wxID_ANY);
		table->Add (_tms_user, 1, wxEXPAND);
		
		add_label_to_sizer (table, panel, _("Password"), true);
		_tms_password = new wxTextCtrl (panel, wxID_ANY);
		table->Add (_tms_password, 1, wxEXPAND);
		
		Config* config = Config::instance ();
		
		_tms_ip->SetValue (std_to_wx (config->tms_ip ()));
		_tms_ip->Bind (wxEVT_COMMAND_TEXT_UPDATED, boost::bind (&TMSPage::tms_ip_changed, this));
		_tms_path->SetValue (std_to_wx (config->tms_path ()));
		_tms_path->Bind (wxEVT_COMMAND_TEXT_UPDATED, boost::bind (&TMSPage::tms_path_changed, this));
		_tms_user->SetValue (std_to_wx (config->tms_user ()));
		_tms_user->Bind (wxEVT_COMMAND_TEXT_UPDATED, boost::bind (&TMSPage::tms_user_changed, this));
		_tms_password->SetValue (std_to_wx (config->tms_password ()));
		_tms_password->Bind (wxEVT_COMMAND_TEXT_UPDATED, boost::bind (&TMSPage::tms_password_changed, this));

		return panel;
	}

private:
	void tms_ip_changed ()
	{
		Config::instance()->set_tms_ip (wx_to_std (_tms_ip->GetValue ()));
	}
	
	void tms_path_changed ()
	{
		Config::instance()->set_tms_path (wx_to_std (_tms_path->GetValue ()));
	}
	
	void tms_user_changed ()
	{
		Config::instance()->set_tms_user (wx_to_std (_tms_user->GetValue ()));
	}
	
	void tms_password_changed ()
	{
		Config::instance()->set_tms_password (wx_to_std (_tms_password->GetValue ()));
	}

	wxTextCtrl* _tms_ip;
	wxTextCtrl* _tms_path;
	wxTextCtrl* _tms_user;
	wxTextCtrl* _tms_password;
};

class KDMEmailPage : public wxPreferencesPage, public Page
{
public:

	KDMEmailPage (wxSize panel_size, int border)
		: Page (panel_size, border)
	{}
	
	wxString GetName () const
	{
		return _("KDM Email");
	}

#ifdef DCPOMATIC_OSX	
	wxBitmap GetLargeIcon () const
	{
		return wxBitmap ("kdm_email", wxBITMAP_TYPE_PNG_RESOURCE);
	}
#endif	

	wxWindow* CreateWindow (wxWindow* parent)
	{
#ifdef DCPOMATIC_OSX		
		/* We have to force both width and height of this one */
		wxPanel* panel = new wxPanel (parent, wxID_ANY, wxDefaultPosition, wxSize (480, 128));
#else
		wxPanel* panel = new wxPanel (parent);
#endif
		wxBoxSizer* s = new wxBoxSizer (wxVERTICAL);
		panel->SetSizer (s);

		wxFlexGridSizer* table = new wxFlexGridSizer (2, DCPOMATIC_SIZER_X_GAP, DCPOMATIC_SIZER_Y_GAP);
		table->AddGrowableCol (1, 1);
		panel->GetSizer()->Add (table, 1, wxEXPAND | wxALL, _border);

		add_label_to_sizer (table, panel, _("Outgoing mail server"), true);
		_mail_server = new wxTextCtrl (panel, wxID_ANY);
		table->Add (_mail_server, 1, wxEXPAND | wxALL);
		
		add_label_to_sizer (table, panel, _("Mail user name"), true);
		_mail_user = new wxTextCtrl (panel, wxID_ANY);
		table->Add (_mail_user, 1, wxEXPAND | wxALL);
		
		add_label_to_sizer (table, panel, _("Mail password"), true);
		_mail_password = new wxTextCtrl (panel, wxID_ANY);
		table->Add (_mail_password, 1, wxEXPAND | wxALL);
		
		wxStaticText* plain = add_label_to_sizer (table, panel, _("(password will be stored on disk in plaintext)"), false);
		wxFont font = plain->GetFont();
		font.SetStyle (wxFONTSTYLE_ITALIC);
		font.SetPointSize (font.GetPointSize() - 1);
		plain->SetFont (font);
		table->AddSpacer (0);

		add_label_to_sizer (table, panel, _("Subject"), true);
		_kdm_subject = new wxTextCtrl (panel, wxID_ANY);
		table->Add (_kdm_subject, 1, wxEXPAND | wxALL);
		
		add_label_to_sizer (table, panel, _("From address"), true);
		_kdm_from = new wxTextCtrl (panel, wxID_ANY);
		table->Add (_kdm_from, 1, wxEXPAND | wxALL);

		add_label_to_sizer (table, panel, _("CC address"), true);
		_kdm_cc = new wxTextCtrl (panel, wxID_ANY);
		table->Add (_kdm_cc, 1, wxEXPAND | wxALL);

		add_label_to_sizer (table, panel, _("BCC address"), true);
		_kdm_bcc = new wxTextCtrl (panel, wxID_ANY);
		table->Add (_kdm_bcc, 1, wxEXPAND | wxALL);
		
		_kdm_email = new wxTextCtrl (panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize (480, 128), wxTE_MULTILINE);
		panel->GetSizer()->Add (_kdm_email, 1, wxEXPAND | wxALL, _border);

		_reset_kdm_email = new wxButton (panel, wxID_ANY, _("Reset to default text"));
		panel->GetSizer()->Add (_reset_kdm_email, 0, wxEXPAND | wxALL, _border);

		Config* config = Config::instance ();
		_mail_server->SetValue (std_to_wx (config->mail_server ()));
		_mail_server->Bind (wxEVT_COMMAND_TEXT_UPDATED, boost::bind (&KDMEmailPage::mail_server_changed, this));
		_mail_user->SetValue (std_to_wx (config->mail_user ()));
		_mail_user->Bind (wxEVT_COMMAND_TEXT_UPDATED, boost::bind (&KDMEmailPage::mail_user_changed, this));
		_mail_password->SetValue (std_to_wx (config->mail_password ()));
		_mail_password->Bind (wxEVT_COMMAND_TEXT_UPDATED, boost::bind (&KDMEmailPage::mail_password_changed, this));
		_kdm_subject->SetValue (std_to_wx (config->kdm_subject ()));
		_kdm_subject->Bind (wxEVT_COMMAND_TEXT_UPDATED, boost::bind (&KDMEmailPage::kdm_subject_changed, this));
		_kdm_from->SetValue (std_to_wx (config->kdm_from ()));
		_kdm_from->Bind (wxEVT_COMMAND_TEXT_UPDATED, boost::bind (&KDMEmailPage::kdm_from_changed, this));
		_kdm_cc->SetValue (std_to_wx (config->kdm_cc ()));
		_kdm_cc->Bind (wxEVT_COMMAND_TEXT_UPDATED, boost::bind (&KDMEmailPage::kdm_cc_changed, this));
		_kdm_bcc->SetValue (std_to_wx (config->kdm_bcc ()));
		_kdm_bcc->Bind (wxEVT_COMMAND_TEXT_UPDATED, boost::bind (&KDMEmailPage::kdm_bcc_changed, this));
		_kdm_email->Bind (wxEVT_COMMAND_TEXT_UPDATED, boost::bind (&KDMEmailPage::kdm_email_changed, this));
		_kdm_email->SetValue (std_to_wx (Config::instance()->kdm_email ()));
		_reset_kdm_email->Bind (wxEVT_COMMAND_BUTTON_CLICKED, boost::bind (&KDMEmailPage::reset_kdm_email, this));

		return panel;
	}

private:
	void mail_server_changed ()
	{
		Config::instance()->set_mail_server (wx_to_std (_mail_server->GetValue ()));
	}
	
	void mail_user_changed ()
	{
		Config::instance()->set_mail_user (wx_to_std (_mail_user->GetValue ()));
	}
	
	void mail_password_changed ()
	{
		Config::instance()->set_mail_password (wx_to_std (_mail_password->GetValue ()));
	}

	void kdm_subject_changed ()
	{
		Config::instance()->set_kdm_subject (wx_to_std (_kdm_subject->GetValue ()));
	}
	
	void kdm_from_changed ()
	{
		Config::instance()->set_kdm_from (wx_to_std (_kdm_from->GetValue ()));
	}

	void kdm_cc_changed ()
	{
		Config::instance()->set_kdm_cc (wx_to_std (_kdm_cc->GetValue ()));
	}

	void kdm_bcc_changed ()
	{
		Config::instance()->set_kdm_bcc (wx_to_std (_kdm_bcc->GetValue ()));
	}
	
	void kdm_email_changed ()
	{
		Config::instance()->set_kdm_email (wx_to_std (_kdm_email->GetValue ()));
	}

	void reset_kdm_email ()
	{
		Config::instance()->reset_kdm_email ();
		_kdm_email->SetValue (wx_to_std (Config::instance()->kdm_email ()));
	}

	wxTextCtrl* _mail_server;
	wxTextCtrl* _mail_user;
	wxTextCtrl* _mail_password;
	wxTextCtrl* _kdm_subject;
	wxTextCtrl* _kdm_from;
	wxTextCtrl* _kdm_cc;
	wxTextCtrl* _kdm_bcc;
	wxTextCtrl* _kdm_email;
	wxButton* _reset_kdm_email;
};

class AdvancedPage : public wxStockPreferencesPage, public Page
{
public:

	AdvancedPage (wxSize panel_size, int border)
		: wxStockPreferencesPage (Kind_Advanced)
		, Page (panel_size, border)
	{}
	
	wxWindow* CreateWindow (wxWindow* parent)
	{
		wxPanel* panel = make_panel (parent);

		wxFlexGridSizer* table = new wxFlexGridSizer (2, DCPOMATIC_SIZER_X_GAP, DCPOMATIC_SIZER_Y_GAP);
		table->AddGrowableCol (1, 1);
		panel->GetSizer()->Add (table, 1, wxALL | wxEXPAND, _border);

		{
			add_label_to_sizer (table, panel, _("Maximum JPEG2000 bandwidth"), true);
			wxBoxSizer* s = new wxBoxSizer (wxHORIZONTAL);
			_maximum_j2k_bandwidth = new wxSpinCtrl (panel);
			s->Add (_maximum_j2k_bandwidth, 1);
			add_label_to_sizer (s, panel, _("Mbit/s"), false);
			table->Add (s, 1);
		}

		_allow_any_dcp_frame_rate = new wxCheckBox (panel, wxID_ANY, _("Allow any DCP frame rate"));
		table->Add (_allow_any_dcp_frame_rate, 1, wxEXPAND | wxALL);
		table->AddSpacer (0);

#ifdef __WXOSX__
		wxStaticText* m = new wxStaticText (panel, wxID_ANY, _("Log:"));
		table->Add (m, 0, wxALIGN_TOP | wxLEFT | wxRIGHT | wxEXPAND | wxALL | wxALIGN_RIGHT, 6);
#else		
		wxStaticText* m = new wxStaticText (panel, wxID_ANY, _("Log"));
		table->Add (m, 0, wxALIGN_TOP | wxLEFT | wxRIGHT | wxEXPAND | wxALL, 6);
#endif		
		
		{
			wxBoxSizer* t = new wxBoxSizer (wxVERTICAL);
			_log_general = new wxCheckBox (panel, wxID_ANY, _("General"));
			t->Add (_log_general, 1, wxEXPAND | wxALL);
			_log_warning = new wxCheckBox (panel, wxID_ANY, _("Warnings"));
			t->Add (_log_warning, 1, wxEXPAND | wxALL);
			_log_error = new wxCheckBox (panel, wxID_ANY, _("Errors"));
			t->Add (_log_error, 1, wxEXPAND | wxALL);
			_log_timing = new wxCheckBox (panel, wxID_ANY, S_("Config|Timing"));
			t->Add (_log_timing, 1, wxEXPAND | wxALL);
			table->Add (t, 0, wxALL, 6);
		}

		Config* config = Config::instance ();
		
		_maximum_j2k_bandwidth->SetRange (1, 500);
		_maximum_j2k_bandwidth->SetValue (config->maximum_j2k_bandwidth() / 1000000);
		_maximum_j2k_bandwidth->Bind (wxEVT_COMMAND_SPINCTRL_UPDATED, boost::bind (&AdvancedPage::maximum_j2k_bandwidth_changed, this));
		_allow_any_dcp_frame_rate->SetValue (config->allow_any_dcp_frame_rate ());
		_allow_any_dcp_frame_rate->Bind (wxEVT_COMMAND_CHECKBOX_CLICKED, boost::bind (&AdvancedPage::allow_any_dcp_frame_rate_changed, this));
		_log_general->SetValue (config->log_types() & Log::TYPE_GENERAL);
		_log_general->Bind (wxEVT_COMMAND_CHECKBOX_CLICKED, boost::bind (&AdvancedPage::log_changed, this));
		_log_warning->SetValue (config->log_types() & Log::TYPE_WARNING);
		_log_warning->Bind (wxEVT_COMMAND_CHECKBOX_CLICKED, boost::bind (&AdvancedPage::log_changed, this));
		_log_error->SetValue (config->log_types() & Log::TYPE_ERROR);
		_log_error->Bind (wxEVT_COMMAND_CHECKBOX_CLICKED, boost::bind (&AdvancedPage::log_changed, this));
		_log_timing->SetValue (config->log_types() & Log::TYPE_TIMING);
		_log_timing->Bind (wxEVT_COMMAND_CHECKBOX_CLICKED, boost::bind (&AdvancedPage::log_changed, this));
		
		return panel;
	}

private:

	void maximum_j2k_bandwidth_changed ()
	{
		Config::instance()->set_maximum_j2k_bandwidth (_maximum_j2k_bandwidth->GetValue() * 1000000);
	}

	void allow_any_dcp_frame_rate_changed ()
	{
		Config::instance()->set_allow_any_dcp_frame_rate (_allow_any_dcp_frame_rate->GetValue ());
	}

	void log_changed ()
	{
		int types = 0;
		if (_log_general->GetValue ()) {
			types |= Log::TYPE_GENERAL;
		}
		if (_log_warning->GetValue ()) {
			types |= Log::TYPE_WARNING;
		}
		if (_log_error->GetValue ())  {
			types |= Log::TYPE_ERROR;
		}
		if (_log_timing->GetValue ()) {
			types |= Log::TYPE_TIMING;
		}
		Config::instance()->set_log_types (types);
	}
	
	wxSpinCtrl* _maximum_j2k_bandwidth;
	wxCheckBox* _allow_any_dcp_frame_rate;
	wxCheckBox* _log_general;
	wxCheckBox* _log_warning;
	wxCheckBox* _log_error;
	wxCheckBox* _log_timing;
};
	
wxPreferencesEditor*
create_config_dialog ()
{
	wxPreferencesEditor* e = new wxPreferencesEditor ();

#ifdef DCPOMATIC_OSX
	/* Width that we force some of the config panels to be on OSX so that
	   the containing window doesn't shrink too much when we select those panels.
	   This is obviously an unpleasant hack.
	*/
	wxSize ps = wxSize (520, -1);
	int const border = 16;
#else
	wxSize ps = wxSize (-1, -1);
	int const border = 8;
#endif
	
	e->AddPage (new GeneralPage (ps, border));
	e->AddPage (new DefaultsPage (ps, border));
	e->AddPage (new EncodingServersPage (ps, border));
	e->AddPage (new ColourConversionsPage (ps, border));
	e->AddPage (new TMSPage (ps, border));
	e->AddPage (new KDMEmailPage (ps, border));
	e->AddPage (new AdvancedPage (ps, border));
	return e;
}
