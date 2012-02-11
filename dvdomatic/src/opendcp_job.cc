#include <stdio.h>
#include "opendcp_job.h"

using namespace std;

OpenDCPJob::OpenDCPJob (Film* f)
	: Job (f)
{

}

void
OpenDCPJob::command (string const & c)
{
	FILE* f = popen (c.c_str(), "r");
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
	if (WEXITSTATUS (r) != 0) {
		set_state (FINISHED_ERROR);
	} else {
		set_state (FINISHED_OK);
	}
}
