#include "make_mxf_job.h"
#include "film.h"

using namespace std;

MakeMXFJob::MakeMXFJob (Film* f, Type t)
	: OpenDCPJob (f)
	, _type (t)
{

}

string
MakeMXFJob::name () const
{
	stringstream s;
	switch (_type) {
	case VIDEO:
		s << "Make video MXF for " << _film->name();
		break;
	case AUDIO:
		s << "Make audio MXF for " << _film->name();
		break;
	}
	
	return s.str ();
}

void
MakeMXFJob::run ()
{
	stringstream c;
	c << "opendcp_mxf -r " << _film->frames_per_second() << " -i ";
	switch (_type) {
	case VIDEO:
		c << _film->dir ("j2c") << " -o " << _film->file ("video.mxf");
		break;
	case AUDIO:
		c << _film->dir ("wavs") << " -o " << _film->file ("audio.mxf");
		break;
	}
	
	command (c.str ());
	_progress.set_progress (1);
}
