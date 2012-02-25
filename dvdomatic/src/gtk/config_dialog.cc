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
#include "lib/config.h"
#include "lib/server.h"
#include "config_dialog.h"
#include "gtk_util.h"

using namespace std;

ConfigDialog::ConfigDialog ()
	: Gtk::Dialog ("DVD-o-matic Configuration")
	, _servers (2, true)
	, _add_server ("Add")
{
	Gtk::Table* t = manage (new Gtk::Table);
	t->set_row_spacings (6);
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
	list<Server*> servers = Config::instance()->servers ();
	for (list<Server*>::iterator i = servers.begin(); i != servers.end(); ++i) {
		int const r = _servers.append_text ((*i)->host_name ());
		stringstream s;
		s << (*i)->threads ();
		_servers.set_text (r, 1, s.str ());
	}

	_add_server.signal_clicked().connect (sigc::mem_fun (*this, &ConfigDialog::add_server_clicked));

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
	list<Server*> servers;
	
	for (size_t i = 0; i < _servers.size(); ++i) {
		servers.push_back (new Server (_servers.get_text (i, 0), atoi (_servers.get_text (i, 1).c_str ())));
	}

	Config::instance()->set_servers (servers);

	Gtk::Dialog::on_response (r);
}

void
ConfigDialog::add_server_clicked ()
{
	int const r = _servers.append_text ("localhost");
	_servers.set_text (r, 1, "1");
}
