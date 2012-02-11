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
