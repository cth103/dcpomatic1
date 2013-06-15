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

#include <fstream>
#include <boost/algorithm/string.hpp>
#include "cross.h"
#ifdef DCPOMATIC_POSIX
#include <unistd.h>
#endif
#ifdef DCPOMATIC_WINDOWS
#include "windows.h"
#endif
#ifdef DCPOMATIC_OSX
#include <sys/sysctl.h>
#endif

using std::pair;
using std::ifstream;
using std::string;

void
dcpomatic_sleep (int s)
{
#ifdef DCPOMATIC_POSIX
	sleep (s);
#endif
#ifdef DCPOMATIC_WINDOWS
	Sleep (s * 1000);
#endif
}

/** @return A pair containing CPU model name and the number of processors */
pair<string, int>
cpu_info ()
{
	pair<string, int> info;
	info.second = 0;
	
#ifdef DCPOMATIC_LINUX
	ifstream f ("/proc/cpuinfo");
	while (f.good ()) {
		string l;
		getline (f, l);
		if (boost::algorithm::starts_with (l, "model name")) {
			string::size_type const c = l.find (':');
			if (c != string::npos) {
				info.first = l.substr (c + 2);
			}
		} else if (boost::algorithm::starts_with (l, "processor")) {
			++info.second;
		}
	}
#endif

#ifdef DCPOMATIC_OSX
	size_t N = sizeof (info.second);
	sysctlbyname ("hw.ncpu", &info.second, &N, 0, 0);
	char buffer[64];
	N = sizeof (buffer);
	if (sysctlbyname ("machdep.cpu.brand_string", buffer, &N, 0, 0) == 0) {
	        info.first = buffer;
        }
#endif		

	return info;
}

