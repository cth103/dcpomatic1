#include <iostream>
#include <boost/filesystem.hpp>
#include "film_viewer.h"
#include "film_editor.h"
#include "job_manager_view.h"
#include "film.h"
#include "format.h"

using namespace std;

int main (int argc, char* argv[])
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

	Film f (argv[1]);
	
	FilmViewer film_view (&f);
	FilmEditor film_editor (&f);
	JobManagerView jobs_view;

	Gtk::Window window;
	window.set_title ("DVD-o-matic");

	Gtk::VBox vbox;
	vbox.set_spacing (12);
	
	Gtk::HBox hbox;
	hbox.set_spacing (12);
	hbox.pack_start (film_editor.get_widget (), false, false);
	hbox.pack_start (film_view.get_widget ());
	vbox.pack_start (hbox, true, true);

	vbox.pack_start (jobs_view.get_widget(), false, false);

	window.add (vbox);
	window.show_all ();
	
	Glib::signal_timeout().connect (sigc::bind_return (sigc::mem_fun (jobs_view, &JobManagerView::update), true), 1000);

	window.maximize ();
	Gtk::Main::run (window);

	return 0;
}
