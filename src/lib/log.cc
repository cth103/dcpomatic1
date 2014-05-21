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

/** @file src/log.cc
 *  @brief A very simple logging class.
 */

#include <time.h>
#include <cstdio>
#include <boost/algorithm/string.hpp>
#include "log.h"
#include "cross.h"
#include "config.h"

#include "i18n.h"

using namespace std;
using boost::algorithm::is_any_of;
using boost::algorithm::split;

int const Log::GENERAL = 0x1;
int const Log::WARNING = 0x2;
int const Log::ERROR   = 0x4;
int const Log::TIMING  = 0x8;

Log::Log ()
	: _types (0)
{
	Config::instance()->Changed.connect (boost::bind (&Log::config_changed, this));
	config_changed ();
}

void
Log::config_changed ()
{
	set_types (Config::instance()->log_types ());
}

/** @param n String to log */
void
Log::log (string message, int type)
{
	boost::mutex::scoped_lock lm (_mutex);

	if ((_types & type) == 0) {
		return;
	}

	time_t t;
	time (&t);
	string a = ctime (&t);

	stringstream s;
	s << a.substr (0, a.length() - 1) << N_(": ");

	if (type & ERROR) {
		s << "ERROR: ";
	}

	if (type & WARNING) {
		s << "WARNING: ";
	}
	
	s << message;
	do_log (s.str ());
}

void
Log::microsecond_log (string m, int t)
{
	boost::mutex::scoped_lock lm (_mutex);

	if ((_types & t) == 0) {
		return;
	}

	struct timeval tv;
	gettimeofday (&tv, 0);

	stringstream s;
	s << tv.tv_sec << N_(":") << tv.tv_usec << N_(" ") << m;
	do_log (s.str ());
}	

void
Log::set_types (int t)
{
	boost::mutex::scoped_lock lm (_mutex);
	_types = t;
}

/** @param A comma-separate list of debug types to enable */
void
Log::set_types (string t)
{
	boost::mutex::scoped_lock lm (_mutex);

	vector<string> types;
	split (types, t, is_any_of (","));

	_types = 0;

	for (vector<string>::const_iterator i = types.begin(); i != types.end(); ++i) {
		if (*i == N_("general")) {
			_types |= GENERAL;
		} else if (*i == N_("warning")) {
			_types |= WARNING;
		} else if (*i == N_("error")) {
			_types |= ERROR;
		} else if (*i == N_("timing")) {
			_types |= TIMING;
		}
	}
}

/** @param file Filename to write log to */
FileLog::FileLog (boost::filesystem::path file)
	: _file (file)
{

}

void
FileLog::do_log (string m)
{
	FILE* f = fopen_boost (_file, "a");
	if (!f) {
		cout << "(could not log to " << _file.string() << "): " << m << "\n";
		return;
	}

	fprintf (f, "%s\n", m.c_str ());
	fclose (f);
}

