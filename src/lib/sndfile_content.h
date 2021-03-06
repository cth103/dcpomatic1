/*
    Copyright (C) 2013-2015 Carl Hetherington <cth@carlh.net>

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

#ifndef DCPOMATIC_SNDFILE_CONTENT_H
#define DCPOMATIC_SNDFILE_CONTENT_H

extern "C" {
#include <libavutil/audioconvert.h>
}
#include "audio_content.h"
#include "audio_stream.h"

namespace cxml {
	class Node;
}

class AudioStream;

class SndfileContent : public AudioContent
{
public:
	SndfileContent (boost::shared_ptr<const Film>, boost::filesystem::path);
	SndfileContent (boost::shared_ptr<const Film>, boost::shared_ptr<const cxml::Node>, int);

	boost::shared_ptr<SndfileContent> shared_from_this () {
		return boost::dynamic_pointer_cast<SndfileContent> (Content::shared_from_this ());
	}

	void examine (boost::shared_ptr<Job>);
	std::string summary () const;
	std::string technical_summary () const;
	void as_xml (xmlpp::Node *) const;
	Time full_length () const;

	/* AudioContent */
	AudioContent::Frame audio_length () const;
	std::vector<AudioStreamPtr> audio_streams () const;

	AudioStreamPtr audio_stream () const {
		return _audio_stream;
	}

	static bool valid_file (boost::filesystem::path);

private:
	boost::shared_ptr<AudioStream> _audio_stream;
	AudioContent::Frame _audio_length;
};

#endif
