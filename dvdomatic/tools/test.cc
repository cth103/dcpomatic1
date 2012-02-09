#include <iostream>
#include "film.h"
#include "format.h"
#include "film_writer.h"
#include "progress.h"

using namespace std;

int main ()
{
	Format::setup_formats ();

	Film f ("/home/carl/Video/Ghostbusters/");
	f.set_content ("DVD_VIDEO/VIDEO_TS/VTS_02_1.VOB");
	f.set_top_crop (75);
	f.set_bottom_crop (75);
	f.set_format (Format::get_from_nickname ("Scope"));
	f.update_thumbs ();
	
	return 0;
}
