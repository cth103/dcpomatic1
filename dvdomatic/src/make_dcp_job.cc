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

#include <boost/filesystem.hpp>
#include "make_dcp_job.h"
#include "film.h"

using namespace std;

MakeDCPJob::MakeDCPJob (Film* f)
	: OpenDCPJob (f)
{

}

string
MakeDCPJob::name () const
{
	stringstream s;
	s << "Make DCP for " << _film->name();
	return s.str ();
}

void
MakeDCPJob::run ()
{
	using namespace boost::filesystem;
	
	stringstream c;

	string const n = _film->dcp_long_name ();
	string const d = _film->dir (n);
	
	c << "cd " << d << " &&"
	  << " opendcp_xml -d -a"
	  << " -t \"" << _film->dcp_pretty_name () << "\""
	  << " -k " << _film->dcp_content_type()->opendcp_name ()
	  << " --reel " << _film->file ("video.mxf") << " " << _film->file ("audio.mxf");

	command (c.str ());

	rename (path (_film->file ("video.mxf")), path (d + "/video.mxf"));
	rename (path (_film->file ("audio.mxf")), path (d + "/audio.mxf"));

	_progress.set_progress (1);
}
