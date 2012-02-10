#include <stdexcept>
#include "demux_job.h"
#include "film_writer.h"
#include "film.h"
#include "format.h"

using namespace std;

DemuxJob::DemuxJob (Film* f)
	: Job (f)
{

}

string
DemuxJob::name () const
{
	stringstream s;
	s << "Demux " << _film->name();
	return s.str ();
}

void
DemuxJob::run ()
{
	try {

		FilmWriter w (_film, &_progress, _film->format()->dci_width(), _film->format()->dci_height(), _film->dir ("tiffs"), _film->dir ("wavs"));
		w.go ();
		set_state (FINISHED_OK);

	} catch (runtime_error& e) {

		set_state (FINISHED_ERROR);

	}
}
