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
#include "film_state.h"
#include "content_type.h"

using namespace std;
using namespace boost;

MakeDCPJob::MakeDCPJob (shared_ptr<const FilmState> s, shared_ptr<const Options> o, Log* l)
	: OpenDCPJob (s, o, l)
{
	
}

string
MakeDCPJob::name () const
{
	stringstream s;
	s << "Make DCP for " << _fs->name;
	return s.str ();
}

void
MakeDCPJob::run ()
{
	assert (!_fs->dcp_long_name.empty());
		
	string const dcp_path = _fs->dir (_fs->dcp_long_name);
	boost::filesystem::remove_all (dcp_path);

	/* Re-make the DCP directory */
	_fs->dir (_fs->dcp_long_name);
	
	stringstream c;
	c << "cd " << dcp_path << " && "
	  << " opendcp_xml -d -a " << _fs->dcp_long_name
	  << " -t \"" << _fs->name << "\""
	  << " -k " << _fs->dcp_content_type->opendcp_name()
	  << " --reel " << _fs->file ("video.mxf") << " " << _fs->file ("audio.mxf");

	command (c.str ());

	boost::filesystem::rename (boost::filesystem::path (_fs->file ("video.mxf")), boost::filesystem::path (dcp_path + "/video.mxf"));
	boost::filesystem::rename (boost::filesystem::path (_fs->file ("audio.mxf")), boost::filesystem::path (dcp_path + "/audio.mxf"));

	set_progress (1);
}
