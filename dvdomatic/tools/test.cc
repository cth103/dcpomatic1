#include "film.h"
#include "format.h"

int main ()
{
	Format::setup_formats ();
	
	Film f ("/home/carl/Films/Ghostbusters/DVD_VIDEO/VIDEO_TS/VTS_02_1.VOB");
	f.set_top_crop (75);
	f.set_bottom_crop (75);
	f.set_format (Format::get (2.39));
	f.make_tiffs ("foo", 200);
	return 0;
}


