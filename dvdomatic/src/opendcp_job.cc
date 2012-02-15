/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <stdio.h>
#include "opendcp_job.h"

using namespace std;

OpenDCPJob::OpenDCPJob (Parameters const * p, Log* l)
	: Job (p, l)
{

}

void
OpenDCPJob::command (string c)
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
						set_progress (float (current) / total);
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
