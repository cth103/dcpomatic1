#include <iostream>
#include "film.h"
#include "format.h"
#include "film_writer.h"
#include "demux_job.h"
#include "job_manager.h"

using namespace std;

int main ()
{
	Format::setup_formats ();

	Film f ("/home/carl/Video/Aurora/");
	DemuxJob* j = new DemuxJob (&f);
	JobManager::instance()->add (j);

	while (1) {
		sleep (1);
	}
	
	return 0;
}
