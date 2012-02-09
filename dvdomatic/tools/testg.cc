#include "film_viewer.h"
#include "film_editor.h"
#include "film.h"
#include "format.h"

int main (int argc, char* argv[])
{
	Format::setup_formats ();
	Gtk::Main kit (argc, argv);

	Film f ("/home/carl/Video/Ghostbusters");
	
	FilmViewer view (&f);
	FilmEditor editor (&f);
	
	Gtk::Window window;
	window.set_title ("DVD-o-matic");

	Gtk::HBox hbox;
	hbox.set_spacing (12);
	hbox.pack_start (editor.get_widget (), false, false);
	hbox.pack_start (view.get_widget ());

	window.add (hbox);
	window.show_all ();
	
	Gtk::Main::run (window);

	return 0;
}
