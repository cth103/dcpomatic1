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

#include <sstream>
#include <iomanip>
#include <iostream>
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

