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

/** @file src/config_dialog.cc
 *  @brief A dialogue to edit DVD-o-matic configuration.
 */

#include <iostream>
#include <boost/lexical_cast.hpp>
#include "lib/config.h"
#include "lib/server.h"
#include "lib/screen.h"
#include "lib/format.h"
#include "config_dialog.h"
#include "gtk_util.h"

using namespace std;
using namespace boost;

ConfigDialog::ConfigDialog ()
	: Gtk::Dialog ("DVD-o-matic Configuration")
	, _servers (2, true)
	, _add_server ("Add server")
	, _add_screen ("Add screen")
{
	Gtk::Table* t = manage (new Gtk::Table);
	t->set_row_spacings (12);
	t->set_col_spacings (6);
	t->set_border_width (6);

	_num_local_encoding_threads.set_range (1, 128);
	_num_local_encoding_threads.set_increments (1, 4);
	_num_local_encoding_threads.set_value (Config::instance()->num_local_encoding_threads ());
	_num_local_encoding_threads.signal_changed().connect (sigc::mem_fun (*this, &ConfigDialog::num_local_encoding_threads_changed));

	_colour_lut.append_text ("sRGB");
	_colour_lut.append_text ("Rec 709");
	_colour_lut.set_active (Config::instance()->colour_lut_index ());
	_colour_lut.signal_changed().connect (sigc::mem_fun (*this, &ConfigDialog::colour_lut_changed));
	
	_j2k_bandwidth.set_range (50, 250);
	_j2k_bandwidth.set_increments (10, 50);
	_j2k_bandwidth.set_value (Config::instance()->j2k_bandwidth() / 1e6);
	_j2k_bandwidth.signal_changed().connect (sigc::mem_fun (*this, &ConfigDialog::j2k_bandwidth_changed));

	_servers.set_column_title (0, "Host name");
	_servers.set_column_title (1, "Threads");
	vector<Server*> servers = Config::instance()->servers ();
	for (vector<Server*>::iterator i = servers.begin(); i != servers.end(); ++i) {
		int const r = _servers.append_text ((*i)->host_name ());
		_servers.set_text (r, 1, lexical_cast<string> ((*i)->threads ()));
	}

	_add_server.signal_clicked().connect (sigc::mem_fun (*this, &ConfigDialog::add_server_clicked));

	_screens_store = Gtk::TreeStore::create (_screens_columns);
	vector<shared_ptr<Screen> > screens = Config::instance()->screens ();
	for (vector<shared_ptr<Screen> >::iterator i = screens.begin(); i != screens.end(); ++i) {
		add_screen_to_store (*i);
	}

	_screens_view.set_model (_screens_store);
	_screens_view.append_column_editable ("Screen", _screens_columns._name);
	_screens_view.append_column ("Format", _screens_columns._format_name);
	_screens_view.append_column_editable ("x", _screens_columns._x);
	_screens_view.append_column_editable ("y", _screens_columns._y);
	_screens_view.append_column_editable ("Width", _screens_columns._width);
	_screens_view.append_column_editable ("Height", _screens_columns._height);

	_add_screen.signal_clicked().connect (sigc::mem_fun (*this, &ConfigDialog::add_screen_clicked));

	int n = 0;
	t->attach (left_aligned_label ("Threads to use for encoding on this host"), 0, 1, n, n + 1);
	t->attach (_num_local_encoding_threads, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Colour look-up table"), 0, 1, n, n + 1);
	t->attach (_colour_lut, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("JPEG2000 bandwidth"), 0, 1, n, n + 1);
	t->attach (_j2k_bandwidth, 1, 2, n, n + 1);
	t->attach (left_aligned_label ("MBps"), 2, 3, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Servers"), 0, 1, n, n + 1);
	t->attach (_servers, 1, 2, n, n + 1);
	Gtk::VBox* b = manage (new Gtk::VBox);
	b->pack_start (_add_server, false, false);
	t->attach (*b, 2, 3, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Screens"), 0, 1, n, n + 1);
	t->attach (_screens_view, 1, 2, n, n + 1);
	b = manage (new Gtk::VBox);
	b->pack_start (_add_screen, false, false);
	t->attach (*b, 2, 3, n, n + 1);
	++n;

	t->show_all ();
	get_vbox()->pack_start (*t);

	get_vbox()->set_border_width (24);

	add_button ("Close", Gtk::RESPONSE_CLOSE);
}

void
ConfigDialog::num_local_encoding_threads_changed ()
{
	Config::instance()->set_num_local_encoding_threads (_num_local_encoding_threads.get_value ());
}

void
ConfigDialog::colour_lut_changed ()
{
	Config::instance()->set_colour_lut_index (_colour_lut.get_active_row_number ());
}

void
ConfigDialog::j2k_bandwidth_changed ()
{
	Config::instance()->set_j2k_bandwidth (_j2k_bandwidth.get_value() * 1e6);
}

void
ConfigDialog::on_response (int r)
{
	vector<Server*> servers;
	for (size_t i = 0; i < _servers.size(); ++i) {
		servers.push_back (new Server (_servers.get_text (i, 0), atoi (_servers.get_text (i, 1).c_str ())));
	}
	Config::instance()->set_servers (servers);

	vector<shared_ptr<Screen> > screens;

	Gtk::TreeModel::Children c = _screens_store->children ();
	for (Gtk::TreeModel::Children::iterator i = c.begin(); i != c.end(); ++i) {

		Gtk::TreeModel::Row r = *i;
		shared_ptr<Screen> s (new Screen (r[_screens_columns._name]));

		Gtk::TreeModel::Children cc = r.children ();
		for (Gtk::TreeModel::Children::iterator j = cc.begin(); j != cc.end(); ++j) {
			Gtk::TreeModel::Row r = *j;
			string const x_ = r[_screens_columns._x];
			string const y_ = r[_screens_columns._y];
			string const width_ = r[_screens_columns._width];
			string const height_ = r[_screens_columns._height];
			s->add_geometry (
				Format::from_nickname (r[_screens_columns._format_nickname]),
				Position (lexical_cast<int> (x_), lexical_cast<int> (y_)),
				Size (lexical_cast<int> (width_), lexical_cast<int> (height_))
				);
		}

		screens.push_back (s);
	}
	
	Config::instance()->set_screens (screens);
	
	Gtk::Dialog::on_response (r);
}

void
ConfigDialog::add_server_clicked ()
{
	int const r = _servers.append_text ("localhost");
	_servers.set_text (r, 1, "1");
}

void
ConfigDialog::add_screen_to_store (shared_ptr<Screen> s)
{
	Gtk::TreeModel::iterator i = _screens_store->append ();
	Gtk::TreeModel::Row r = *i;
	r[_screens_columns._name] = s->name ();

	Screen::GeometryMap geoms = s->geometries ();
	for (Screen::GeometryMap::const_iterator j = geoms.begin(); j != geoms.end(); ++j) {
		i = _screens_store->append (r.children ());
		Gtk::TreeModel::Row c = *i;
		c[_screens_columns._format_name] = j->first->name ();
		c[_screens_columns._format_nickname] = j->first->nickname ();
		c[_screens_columns._x] = lexical_cast<string> (j->second.position.x);
		c[_screens_columns._y] = lexical_cast<string> (j->second.position.y);
		c[_screens_columns._width] = lexical_cast<string> (j->second.size.width);
		c[_screens_columns._height] = lexical_cast<string> (j->second.size.height);
	}
}

void
ConfigDialog::add_screen_clicked ()
{
	shared_ptr<Screen> s (new Screen ("New Screen"));
	vector<Format const *> f = Format::all ();
	for (vector<Format const *>::iterator i = f.begin(); i != f.end(); ++i) {
		s->add_geometry (*i, Position (0, 0), Size (2048, 1080));
	}
	
	add_screen_to_store (s);
}
