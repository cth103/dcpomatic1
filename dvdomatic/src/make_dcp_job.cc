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
#include "parameters.h"

using namespace std;

MakeDCPJob::MakeDCPJob (Parameters const * p, Options const * o, Log* l)
	: OpenDCPJob (p, o, l)
{
	
}

string
MakeDCPJob::name () const
{
	stringstream s;
	s << "Make DCP for " << _par->film_name;
	return s.str ();
}

void
MakeDCPJob::run ()
{
	boost::filesystem::remove_all (_par->dcp_path);
	
	stringstream c;
	c << "cd " << _par->dcp_path << " &&"
	  << " opendcp_xml -d -a " << _par->dcp_long_name
	  << " -t \"" << _par->film_name << "\""
	  << " -k " << _par->dcp_content_type_name
	  << " --reel " << _par->video_mxf_path << " " << _par->audio_mxf_path;

	command (c.str ());

	boost::filesystem::rename (boost::filesystem::path (_par->video_mxf_path), boost::filesystem::path (_par->dcp_path + "/video.mxf"));
	boost::filesystem::rename (boost::filesystem::path (_par->audio_mxf_path), boost::filesystem::path (_par->dcp_path + "/audio.mxf"));

	set_progress (1);
}
