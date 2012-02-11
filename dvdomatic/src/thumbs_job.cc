#include "thumbs_job.h"
#include "film.h"

using namespace std;

ThumbsJob::ThumbsJob (Film* f)
	: Job (f)
{
	
}

string
ThumbsJob::name () const
{
	stringstream s;
	s << "Update thumbs for " << _film->name();
	return s.str ();
}

void
ThumbsJob::run ()
{
	_film->update_thumbs_non_gui (&_progress);
	set_state (FINISHED_OK);
}
