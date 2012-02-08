#include "film.h"
#include "format.h"
#include "film_writer.h"

int main ()
{
	Format::setup_formats ();
	
//	Film f ("/home/carl/Films/Ghostbusters/DVD_VIDEO/VIDEO_TS/VTS_02_1.VOB");
	Film f ("/home/carl/Films/Ghostbusters.avi");
	f.set_top_crop (75);
	f.set_bottom_crop (75);
	f.set_format (Format::get ("Test"));

	FilmWriter w (&f, "foo", "bar", 4000);
	return 0;
}
