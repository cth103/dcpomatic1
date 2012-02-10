#include "film_viewer.h"
#include "film_editor.h"
#include "film.h"
#include "format.h"
#include "job_manager_view.h"

int main (int argc, char* argv[])
{
	Format::setup_formats ();
	Gtk::Main kit (argc, argv);

	Film f ("/home/carl/Video/Aurora");

	JobManagerView job_manager_view;
	FilmViewer film_view (&f);
	FilmEditor film_editor (&f);
	
	Gtk::Window window;
	window.set_title ("DVD-o-matic");

	Gtk::HBox hbox;
	hbox.set_spacing (12);
	hbox.pack_start (film_editor.get_widget (), false, false);
	hbox.pack_start (film_view.get_widget ());
	hbox.pack_start (job_manager_view.get_widget ());

	window.add (hbox);
	window.show_all ();

	Glib::signal_timeout().connect (sigc::bind_return (sigc::mem_fun (job_manager_view, &JobManagerView::update), true), 1000);
	
	Gtk::Main::run (window);

	return 0;
}
