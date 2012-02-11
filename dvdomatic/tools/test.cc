#include <iostream>
#include "film.h"
#include "format.h"
#include "tiff_transcoder.h"
#include "transcode_job.h"
#include "job_manager.h"

using namespace std;

int main ()
{
	Format::setup_formats ();
	ContentType::setup_content_types ();

	Film f ("/home/carl/Video/Aurora/");
	TranscodeJob* j = new TranscodeJob (&f);
	JobManager::instance()->add (j);

	while (1) {
		sleep (1);
	}
	
	return 0;
}
