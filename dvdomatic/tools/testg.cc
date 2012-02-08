#include "film_viewer.h"
#include "film.h"
#include "format.h"

int main (int argc, char* argv[])
{
	Format::setup_formats ();
	Gtk::Main kit (argc, argv);

	Film f ("/home/carl/Films/Ghostbusters");
	
	FilmViewer view (&f);
	
	Gtk::Window window;
	window.set_title ("DVD-o-matic");
	window.add (view.get_container ());
	window.show_all ();
	
	Gtk::Main::run (window);

	return 0;
}
