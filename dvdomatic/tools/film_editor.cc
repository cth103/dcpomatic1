#include <boost/filesystem.hpp>
#include "film_viewer.h"
#include "film_editor.h"
#include "film.h"
#include "format.h"

using namespace std;

int main (int argc, char* argv[])
{
	Format::setup_formats ();
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
	
	FilmViewer view (&f);
	FilmEditor editor (&f);

	Gtk::Window window;
	window.set_title ("DVD-o-matic: Film Editor");

	Gtk::HBox hbox;
	hbox.set_spacing (12);
	hbox.pack_start (editor.get_widget (), false, false);
	hbox.pack_start (view.get_widget ());

	window.add (hbox);
	window.show_all ();
	
	Gtk::Main::run (window);

	return 0;
}
