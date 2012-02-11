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

#include "make_mxf_job.h"
#include "film.h"

using namespace std;

MakeMXFJob::MakeMXFJob (Film* f, Type t)
	: OpenDCPJob (f)
	, _type (t)
{

}

string
MakeMXFJob::name () const
{
	stringstream s;
	switch (_type) {
	case VIDEO:
		s << "Make video MXF for " << _film->name();
		break;
	case AUDIO:
		s << "Make audio MXF for " << _film->name();
		break;
	}
	
	return s.str ();
}

void
MakeMXFJob::run ()
{
	stringstream c;
	c << "opendcp_mxf -r " << _film->frames_per_second() << " -i ";
	switch (_type) {
	case VIDEO:
		c << _film->dir ("j2c") << " -o " << _film->file ("video.mxf");
		break;
	case AUDIO:
		c << _film->dir ("wavs") << " -o " << _film->file ("audio.mxf");
		break;
	}
	
	command (c.str ());
	_progress.set_progress (1);
}
