#include "make_video_mxf_job.h"

using namespace std;

MakeVideoMXFJob::MakeVideoMXFJob (Film* f)
	: OpenDCPJob (f)
{

}

string
MakeVideoMXFJob::name () const
{
	stringstream s;
	s << "Make video MXF for " << _film->name();
	return s.str ();
}

void
MakeVideoMXFJob::run ()
{
	stringstream c;
	c << "opendcp_mxf -r " << _film->frames_per_second() << " -i " << _film->dir ("j2c") << " -o " << _film->file ("video.mxf");
	command (c);
}
