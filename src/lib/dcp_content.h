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

/** @file  src/lib/dcp_content.h
 *  @brief DCPContent class.
 */

#include <libcxml/cxml.h>
#include "video_content.h"
#include "single_stream_audio_content.h"
#include "subtitle_content.h"

/** @class DCPContent
 *  @brief An existing DCP used as input.
 */
class DCPContent : public VideoContent, public SingleStreamAudioContent, public SubtitleContent
{
public:
	DCPContent (boost::shared_ptr<const Film> f, boost::filesystem::path p);
	DCPContent (boost::shared_ptr<const Film> f, cxml::ConstNodePtr, int version);

	boost::shared_ptr<DCPContent> shared_from_this () {
		return boost::dynamic_pointer_cast<DCPContent> (Content::shared_from_this ());
	}

	DCPTime full_length () const;
	
	void examine (boost::shared_ptr<Job>);
	std::string summary () const;
	std::string technical_summary () const;
	void as_xml (xmlpp::Node *) const;
	std::string identifier () const;

	/* SubtitleContent */
	bool has_subtitles () const;
	
	boost::filesystem::path directory () const {
		boost::mutex::scoped_lock lm (_mutex);
		return _directory;
	}

private:
	void read_directory (boost::filesystem::path);
	
	std::string _name;
	bool _has_subtitles;
	boost::filesystem::path _directory;
};
