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

#include <libxml++/libxml++.h>
#include <libcxml/cxml.h>
#include <libdcp/raw_convert.h>
#include "audio_mapping.h"
#include "util.h"
#include "md5_digester.h"

using std::list;
using std::cout;
using std::string;
using std::vector;
using std::min;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using libdcp::raw_convert;

AudioMapping::AudioMapping ()
	: _content_channels (0)
{

}

/** Create a default AudioMapping for a given channel count.
 *  @param c Number of channels.
 */
AudioMapping::AudioMapping (int c)
{
	setup (c);
}

void
AudioMapping::setup (int c)
{
	_content_channels = c;
	
	_gain.resize (_content_channels);
	for (int i = 0; i < _content_channels; ++i) {
		_gain[i].resize (MAX_DCP_AUDIO_CHANNELS);
	}
}

void
AudioMapping::make_default ()
{
	for (int i = 0; i < _content_channels; ++i) {
		for (int j = 0; j < MAX_DCP_AUDIO_CHANNELS; ++j) {
			_gain[i][j] = 0;
		}
	}

	if (_content_channels == 1) {
		/* Mono -> Centre */
		set (0, libdcp::CENTRE, 1);
	} else {
		/* 1:1 mapping */
		for (int i = 0; i < min (_content_channels, MAX_DCP_AUDIO_CHANNELS); ++i) {
			set (i, static_cast<libdcp::Channel> (i), 1);
		}
	}
}

AudioMapping::AudioMapping (shared_ptr<const cxml::Node> node, int state_version)
{
	setup (node->number_child<int> ("ContentChannels"));

	if (state_version <= 5) {
		/* Old-style: on/off mapping */
		list<cxml::NodePtr> const c = node->node_children ("Map");
		for (list<cxml::NodePtr>::const_iterator i = c.begin(); i != c.end(); ++i) {
			set ((*i)->number_child<int> ("ContentIndex"), static_cast<libdcp::Channel> ((*i)->number_child<int> ("DCP")), 1);
		}
	} else {
		list<cxml::NodePtr> const c = node->node_children ("Gain");
		for (list<cxml::NodePtr>::const_iterator i = c.begin(); i != c.end(); ++i) {
			set (
				(*i)->number_attribute<int> ("Content"),
				static_cast<libdcp::Channel> ((*i)->number_attribute<int> ("DCP")),
				raw_convert<float> ((*i)->content ())
				);
		}
	}
}

void
AudioMapping::set (int c, libdcp::Channel d, float g)
{
	_gain[c][d] = g;
}

float
AudioMapping::get (int c, libdcp::Channel d) const
{
	return _gain[c][d];
}

void
AudioMapping::as_xml (xmlpp::Node* node) const
{
	node->add_child ("ContentChannels")->add_child_text (raw_convert<string> (_content_channels));

	for (int c = 0; c < _content_channels; ++c) {
		for (int d = 0; d < MAX_DCP_AUDIO_CHANNELS; ++d) {
			xmlpp::Element* t = node->add_child ("Gain");
			t->set_attribute ("Content", raw_convert<string> (c));
			t->set_attribute ("DCP", raw_convert<string> (d));
			t->add_child_text (raw_convert<string> (get (c, static_cast<libdcp::Channel> (d))));
		}
	}
}

/** @return a string which is unique for a given AudioMapping configuration, for
 *  differentiation between different AudioMappings.
 */
string
AudioMapping::digest () const
{
	MD5Digester digester;
	digester.add (_content_channels);
	for (int i = 0; i < _content_channels; ++i) {
		for (int j = 0; j < MAX_DCP_AUDIO_CHANNELS; ++j) {
			digester.add (_gain[i][j]);
		}
	}

	return digester.get ();
}

bool
AudioMapping::has_anything_mapped () const
{
	static float const minus_96_db = 0.000015849;
	
	for (vector<vector<float> >::const_iterator i = _gain.begin(); i != _gain.end(); ++i) {
		for (vector<float>::const_iterator j = i->begin(); j != i->end(); ++j) {
			if (abs (*j) > minus_96_db) {
				return true;
			}
		}
	}

	return false;
}
