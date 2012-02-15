/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>
    Copyright (C) 2000-2007 Paul Davis

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

#include <sstream>
#include <iomanip>
#include <iostream>
#include <execinfo.h>
#include <cxxabi.h>
#include "util.h"

using namespace std;

/** Convert some number of seconds to a string representation
 *  in hours, minutes and seconds.
 *
 *  @param s Seconds.
 *  @return String of the form H:M:S (where H is hours, M
 *  is minutes and S is seconds).
 */
 
string
seconds_to_hms (int s)
{
	int m = s / 60;
	s -= (m * 60);
	int h = m / 60;
	m -= (h * 60);

	stringstream hms;
	hms << h << ":";
	hms.width (2);
	hms << setfill ('0') << m << ":";
	hms.width (2);
	hms << setfill ('0') << s;

	return hms.str ();
}

Gtk::Label &
left_aligned_label (string const & t)
{
	Gtk::Label* l = Gtk::manage (new Gtk::Label (t));
	l->set_alignment (0, 0.5);
	return *l;
}

static string
demangle (string const & l)
{
	string::size_type const b = l.find_first_of ("(");
	if (b == string::npos) {
		return l;
	}

	string::size_type const p = l.find_last_of ("+");
	if (p == string::npos) {
		return l;
	}

	if ((p - b) <= 1) {
		return l;
	}
	
	string const fn = l.substr (b + 1, p - b - 1);

	int status;
	try {
		
		char* realname = abi::__cxa_demangle (fn.c_str(), 0, 0, &status);
		string d (realname);
		free (realname);
		return d;
		
	} catch (exception) {
		
	}
	
	return l;
}

void
stacktrace (ostream& out, int levels)
{
	void *array[200];
	size_t size;
	char **strings;
	size_t i;
     
	size = backtrace (array, 200);
	strings = backtrace_symbols (array, size);
     
	if (strings) {
		for (i = 0; i < size && (levels == 0 || i < size_t(levels)); i++) {
			out << "  " << demangle (strings[i]) << endl;
		}
		
		free (strings);
	}
}

string
audio_sample_format_to_string (AVSampleFormat s)
{
	/** Our sample format handling is not exactly complete */
	
	switch (s) {
	case AV_SAMPLE_FMT_S16:
		return "S16";
	default:
		break;
	}

	return "Unknown";
}

AVSampleFormat
audio_sample_format_from_string (string const & s)
{
	if (s == "S16") {
		return AV_SAMPLE_FMT_S16;
	}

	return AV_SAMPLE_FMT_NONE;
}
