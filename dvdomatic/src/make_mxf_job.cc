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

#include <iostream>
#include "make_mxf_job.h"
#include "film.h"
#include "parameters.h"

using namespace std;

MakeMXFJob::MakeMXFJob (Parameters const * p, Log* l, Type t)
	: OpenDCPJob (p, l)
	, _type (t)
{

}

string
MakeMXFJob::name () const
{
	stringstream s;
	switch (_type) {
	case VIDEO:
		s << "Make video MXF for " << _par->film_name;
		break;
	case AUDIO:
		s << "Make audio MXF for " << _par->film_name;
		break;
	}
	
	return s.str ();
}

void
MakeMXFJob::run ()
{
	float fps = _par->frames_per_second;
	
	/* XXX: experimental hack; round FPS for audio MXFs */
	if (_type == AUDIO) {
		fps = rintf (fps);
	}

	stringstream c;
	c << "opendcp_mxf -r " << fps << " -i ";
	switch (_type) {
	case VIDEO:
		c << _par->video_out_path () << " -o " << _par->video_mxf_path;
		break;
	case AUDIO:
		c << _par->audio_out_path () << " -o " << _par->audio_mxf_path;
		break;
	}

	cout << c.str() << "\n";
	
	command (c.str ());
	set_progress (1);
}
