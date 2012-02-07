#include "film.h"

int main ()
{
	Film f ("/home/carl/Films/Ghostbusters/DVD_VIDEO/VIDEO_TS/VTS_02_1.VOB");
	f.set_top_crop (75);
	f.set_bottom_crop (75);
	f.make_tiffs ("foo", 200);
	return 0;
}


