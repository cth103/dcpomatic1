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
#include "filter.h"

using namespace std;

static Gtk::Window* window = 0;
static FilmViewer* film_viewer = 0;
static FilmEditor* film_editor = 0;
static Film* film = 0;

class FilmChangedDialog : public Gtk::MessageDialog
{
public:
	FilmChangedDialog ()
		: Gtk::MessageDialog ("", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE)
	{
		stringstream s;
		s << "Save changes to film \"" << film->name() << "\" before closing?";
		set_message (s.str ());
		add_button ("Close without saving", Gtk::RESPONSE_NO);
		add_button ("Cancel", Gtk::RESPONSE_CANCEL);
		add_button ("Save", Gtk::RESPONSE_YES);
	}
};

bool
maybe_save_then_delete_film ()
{
	if (film->dirty ()) {
		FilmChangedDialog d;
		switch (d.run ()) {
		case Gtk::RESPONSE_CANCEL:
			return true;
		case Gtk::RESPONSE_YES:
			film->write_metadata ();
			break;
		case Gtk::RESPONSE_NO:
			return false;
		}
	}
	
	delete film;
	film = 0;
	return false;
}

void
file_new ()
{
	Gtk::FileChooserDialog c (*window, "New Film", Gtk::FILE_CHOOSER_ACTION_CREATE_FOLDER);
	c.add_button ("Cancel", Gtk::RESPONSE_CANCEL);
	c.add_button ("Create", Gtk::RESPONSE_ACCEPT);

	int const r = c.run ();
	if (r == Gtk::RESPONSE_ACCEPT) {
		if (maybe_save_then_delete_film ()) {
			return;
		}
		Film* film = new Film (c.get_filename ());
		film_viewer->set_film (film);
		film_editor->set_film (film);
	}
}

void
file_open ()
{
	Gtk::FileChooserDialog c (*window, "Open Film", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	c.add_button ("Cancel", Gtk::RESPONSE_CANCEL);
	c.add_button ("Open", Gtk::RESPONSE_ACCEPT);

	int const r = c.run ();
	if (r == Gtk::RESPONSE_ACCEPT) {
		if (maybe_save_then_delete_film ()) {
			return;
		}
		film = new Film (c.get_filename ());
		film_viewer->set_film (film);
		film_editor->set_film (film);
	}
}

void
file_save ()
{
	film->write_metadata ();
}

void
file_quit ()
{
	if (maybe_save_then_delete_film ()) {
		return;
	}
	
	Gtk::Main::quit ();
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
	file_items.push_back (MenuElem ("New...", sigc::ptr_fun (file_new)));
	file_items.push_back (MenuElem ("Open...", sigc::ptr_fun (file_open)));
	file_items.push_back (SeparatorElem ());
	file_items.push_back (MenuElem ("Save", sigc::ptr_fun (file_save)));
	file_items.push_back (SeparatorElem ());
	file_items.push_back (MenuElem ("Quit", sigc::ptr_fun (file_quit)));

	Gtk::Menu* edit = manage (new Gtk::Menu);
	MenuList& edit_items (edit->items ());
	edit_items.push_back (MenuElem ("Preferences...", sigc::ptr_fun (edit_preferences)));
	
	MenuList& items (m.items ());
	items.push_back (MenuElem ("File", *file));
	items.push_back (MenuElem ("Edit", *edit));
}

bool
window_closed (GdkEventAny *)
{
	if (maybe_save_then_delete_film ()) {
		return true;
	}

	return false;
}

int
main (int argc, char* argv[])
{
	Format::setup_formats ();
	Filter::setup_filters ();
	ContentType::setup_content_types ();
	
	Gtk::Main kit (argc, argv);

	if (argc == 2 && boost::filesystem::is_directory (argv[1])) {
		film = new Film (argv[1]);
	}

	window = new Gtk::Window ();
	window->signal_delete_event().connect (sigc::ptr_fun (window_closed));
	
	film_viewer = new FilmViewer (film);
	film_editor = new FilmEditor (film);
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
