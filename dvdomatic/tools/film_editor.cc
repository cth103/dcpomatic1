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

#include <iostream>
#include <boost/filesystem.hpp>
#include "film_viewer.h"
#include "film_editor.h"
#include "job_manager_view.h"
#include "film.h"
#include "format.h"
#include "config_dialog.h"
#include "config.h"

using namespace std;

static Gtk::Window* window = 0;
static FilmViewer* film_viewer = 0;
static FilmEditor* film_editor = 0;

void
file_open ()
{
	Gtk::FileChooserDialog c (*window, "Open Film", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	c.add_button ("Cancel", Gtk::RESPONSE_CANCEL);
	c.add_button ("Open", Gtk::RESPONSE_ACCEPT);

	int const r = c.run ();
	if (r == Gtk::RESPONSE_ACCEPT) {
		Film* film = new Film (c.get_filename ());
		cout << "Load film " << film << "\n";
		film_viewer->set_film (film);
		film_editor->set_film (film);
	}
		
}

void
edit_preferences ()
{
	ConfigDialog d;
	d.run ();
	Config::instance()->write ();
}

void
setup_menu (Gtk::MenuBar& m)
{
	using namespace Gtk::Menu_Helpers;

	Gtk::Menu* file = manage (new Gtk::Menu);
	MenuList& file_items (file->items ());
	file_items.push_back (MenuElem ("Open...", sigc::ptr_fun (file_open)));

	Gtk::Menu* edit = manage (new Gtk::Menu);
	MenuList& edit_items (edit->items ());
	edit_items.push_back (MenuElem ("Preferences...", sigc::ptr_fun (edit_preferences)));
	
	MenuList& items (m.items ());
	items.push_back (MenuElem ("File", *file));
	items.push_back (MenuElem ("Edit", *edit));
}

int
main (int argc, char* argv[])
{
	Format::setup_formats ();
	ContentType::setup_content_types ();
	
	Gtk::Main kit (argc, argv);

	if (argc < 2) {
		cerr << "Syntax: " << argv[0] << " <film>\n";
		exit (EXIT_FAILURE);
	}

	if (!boost::filesystem::is_directory (argv[1])) {
		cerr << argv[0] << ": could not find film " << argv[1] << "\n";
		exit (EXIT_FAILURE);
	}

	Film film (argv[1]);

	window = new Gtk::Window ();
	
	film_viewer = new FilmViewer (&film);
	film_editor = new FilmEditor (&film);
	JobManagerView jobs_view;

	window->set_title ("DVD-o-matic");

	Gtk::VBox vbox;

	Gtk::MenuBar menu_bar;
	vbox.pack_start (menu_bar, false, false);
	setup_menu (menu_bar);
	
	Gtk::HBox hbox;
	hbox.set_spacing (12);
	hbox.pack_start (film_editor->get_widget (), false, false);
	hbox.pack_start (film_viewer->get_widget ());
	vbox.pack_start (hbox, true, true);

	vbox.pack_start (jobs_view.get_widget(), false, false);

	window->add (vbox);
	window->show_all ();
	
	Glib::signal_timeout().connect (sigc::bind_return (sigc::mem_fun (jobs_view, &JobManagerView::update), true), 1000);

	window->maximize ();
	Gtk::Main::run (*window);

	return 0;
}
