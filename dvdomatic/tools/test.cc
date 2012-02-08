#include <boost/thread/thread.hpp>
#include <iostream>
#include "film.h"
#include "format.h"
#include "film_writer.h"
#include "progress.h"

using namespace std;

void
thread (Film* f, Progress* p)
{
	FilmWriter w (f, "/home/carl/Video/Ghostbusters/tiffs", "/home/carl/Video/Ghostbusters/wavs", p);
}

int main ()
{
	Format::setup_formats ();

	Film f ("/home/carl/Video/Ghostbusters/DVD_VIDEO/VIDEO_TS/VTS_02_1.VOB");
	f.set_top_crop (75);
	f.set_bottom_crop (75);
	f.set_format (Format::get ("Scope"));

	Progress p;
	boost::thread t (boost::bind (&thread, &f, &p));

	while (1) {
		sleep (1);
		cout << (p.get_fraction() * 100) << "%\n";
	}

	return 0;
}
