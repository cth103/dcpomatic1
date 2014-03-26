/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#include <iomanip>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include "lib/film.h"
#include "lib/config.h"
#include "properties_dialog.h"
#include "wx_util.h"

using std::string;
using std::stringstream;
using std::fixed;
using std::setprecision;
using boost::shared_ptr;
using boost::lexical_cast;

PropertiesDialog::PropertiesDialog (wxWindow* parent, shared_ptr<Film> film)
	: TableDialog (parent, _("Film Properties"), 2, false)
	, _film (film)
{
	add (_("Frames"), true);
	_frames = add (new wxStaticText (this, wxID_ANY, wxT ("")));

	add (_("Disk space required"), true);
	_disk = add (new wxStaticText (this, wxID_ANY, wxT ("")));

	add (_("Frames already encoded"), true);
	_encoded = add (new ThreadedStaticText (this, _("counting..."), boost::bind (&PropertiesDialog::frames_already_encoded, this)));
	_encoded->Finished.connect (boost::bind (&PropertiesDialog::layout, this));
	
	_frames->SetLabel (std_to_wx (lexical_cast<string> (_film->time_to_video_frames (_film->length()))));
	double const disk = double (_film->required_disk_space()) / 1073741824.0f;
	stringstream s;
	s << fixed << setprecision (1) << disk << wx_to_std (_("Gb"));
	_disk->SetLabel (std_to_wx (s.str ()));

	layout ();
}

string
PropertiesDialog::frames_already_encoded () const
{
	stringstream u;
	try {
		u << _film->encoded_frames ();
	} catch (boost::thread_interrupted &) {
		return "";
	}
	
	if (_film->length()) {
		/* XXX: encoded_frames() should check which frames have been encoded */
		u << " (" << (_film->encoded_frames() * 100 / _film->time_to_video_frames (_film->length())) << "%)";
	}
	return u.str ();
}
