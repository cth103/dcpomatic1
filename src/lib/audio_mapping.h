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

#ifndef DCPOMATIC_AUDIO_MAPPING_H
#define DCPOMATIC_AUDIO_MAPPING_H

#include <vector>
#include <list>
#include <libdcp/types.h>
#include <boost/shared_ptr.hpp>

namespace xmlpp {
	class Node;
}

namespace cxml {
	class Node;
}

/** A many-to-many mapping from some content channels to DCP channels.
 *  The number of content channels is set on construction and fixed,
 *  and then each of those content channels are mapped to each DCP channel
 *  by a linear gain.
 */
class AudioMapping
{
public:
	AudioMapping ();
	AudioMapping (int);
	AudioMapping (boost::shared_ptr<const cxml::Node>, int);

	/* Default copy constructor is fine */

	void as_xml (xmlpp::Node *) const;

	void make_default (int use);
	void make_zero ();

	void set (int, libdcp::Channel, float);
	float get (int, libdcp::Channel) const;

	int content_channels () const {
		return _content_channels;
	}

	void set_name (int channel, std::string name);
	std::string name (int channel) const {
		return _name[channel];
	}

	std::string digest () const;

	std::list<libdcp::Channel> mapped_dcp_channels () const;
	void unmap_all ();

private:
	void setup (int);

	int _content_channels;
	std::vector<std::vector<float> > _gain;
	std::vector<std::string> _name;
};

#endif
