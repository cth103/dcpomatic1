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

/** @file src/config_dialog.h
 *  @brief A dialogue to edit DVD-o-matic configuration.
 */

#include <gtkmm.h>

/** @class ConfigDialog
 *  @brief A dialogue to edit DVD-o-matic configuration.
 */
class ConfigDialog : public Gtk::Dialog
{
public:
	ConfigDialog ();

private:
	void on_response (int);
	
	void num_local_encoding_threads_changed ();
	void colour_lut_changed ();
	void j2k_bandwidth_changed ();
	void add_server_clicked ();

	Gtk::SpinButton _num_local_encoding_threads;
	Gtk::ComboBoxText _colour_lut;
	Gtk::SpinButton _j2k_bandwidth;
	Gtk::ListViewText _servers;
	Gtk::Button _add_server;
};

