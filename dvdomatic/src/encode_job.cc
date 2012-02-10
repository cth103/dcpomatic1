#include <stdio.h>
#include "encode_job.h"
#include "film.h"

using namespace std;

EncodeJob::EncodeJob (Film* f)
	: Job (f)
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
	FILE* f = popen (c.str().c_str(), "r");
	if (f == 0) {
		set_state (FINISHED_ERROR);
		return;
	}

	while (!feof (f)) {
		char buf[256];
		if (fscanf (f, "%s\n", buf)) {
			string s (buf);
			if (s[0] == '[' && s[s.length() - 1] == ']') {
				size_t const slash = s.find ('/');
				if (slash != string::npos) {
					int const current = atoi (s.substr(1, slash).c_str ());
					int const total = atoi (s.substr(slash + 1, s.length() - 1).c_str ());
					if (total > 0) {
						_progress.set_progress (float (current) / total);
					}
				}
			}
		}
	}

	int const r = pclose (f);
	if (r == -1) {
		set_state (FINISHED_ERROR);
	} else {
		set_state (FINISHED_OK);
	}
}

