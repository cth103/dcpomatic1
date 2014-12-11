/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#include "dcp_subtitle_content.h"
#include <dcp/subtitle_content.h>
#include <dcp/raw_convert.h>

#include "i18n.h"

using std::string;
using std::list;
using boost::shared_ptr;
using dcp::raw_convert;

DCPSubtitleContent::DCPSubtitleContent (shared_ptr<const Film> film, boost::filesystem::path path)
	: Content (film, path)
	, SubtitleContent (film, path)
{
	
}

DCPSubtitleContent::DCPSubtitleContent (shared_ptr<const Film> film, cxml::ConstNodePtr node, int version)
	: Content (film, node)
	, SubtitleContent (film, node, version)
	, _length (node->number_child<DCPTime::Type> ("Length"))
{

}

void
DCPSubtitleContent::examine (shared_ptr<Job> job, bool calculate_digest)
{
	Content::examine (job, calculate_digest);
	dcp::SubtitleContent sc (path (0), false);
	_subtitle_language = sc.language ();
	_length = DCPTime::from_seconds (sc.latest_subtitle_out().to_seconds ());
}

DCPTime
DCPSubtitleContent::full_length () const
{
	/* XXX: this assumes that the timing of the subtitle file is appropriate
	   for the DCP's frame rate.
	*/
	return _length;
}

string
DCPSubtitleContent::summary () const
{
	return path_summary() + " " + _("[subtitles]");
}

string
DCPSubtitleContent::technical_summary () const
{
	return Content::technical_summary() + " - " + _("DCP XML subtitles");
}
      
string
DCPSubtitleContent::information () const
{

}

void
DCPSubtitleContent::as_xml (xmlpp::Node* node) const
{
	node->add_child("Type")->add_child_text ("DCPSubtitle");
	Content::as_xml (node);
	SubtitleContent::as_xml (node);
	node->add_child("Length")->add_child_text (raw_convert<string> (_length.get ()));
}
