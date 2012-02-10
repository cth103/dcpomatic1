#include "encode_job.h"
#include "film.h"

using namespace std;

EncodeJob::EncodeJob (Film* f)
	: OpenDCPJob (f)
{

}

string
EncodeJob::name () const
{
	stringstream s;
	s << "Encode " << _film->name();
	return s.str ();
}

void
EncodeJob::run ()
{
	stringstream c;
	c << "opendcp_j2k -r " << _film->frames_per_second() << " -i " << _film->dir ("tiffs") << " -o " << _film->dir ("j2c");
	command (c.str ());
}
