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

#include <libcxml/cxml.h>
#include "raw_convert.h"
#include "sndfile_content.h"
#include "sndfile_decoder.h"
#include "film.h"
#include "compose.hpp"
#include "job.h"
#include "util.h"
#include "safe_stringstream.h"
#include "audio_stream.h"

#include "i18n.h"

using std::string;
using std::cout;
using std::vector;
using boost::shared_ptr;

SndfileContent::SndfileContent (shared_ptr<const Film> f, boost::filesystem::path p)
	: Content (f, p)
	, AudioContent (f, p)
	, _audio_length (0)
{

}

SndfileContent::SndfileContent (shared_ptr<const Film> f, shared_ptr<const cxml::Node> node, int version)
	: Content (f, node)
	, AudioContent (f, node)
	, _audio_stream (new AudioStream (node->number_child<int> ("AudioFrameRate"), AudioMapping (node->node_child ("AudioMapping"), version)))
	, _audio_length (node->number_child<AudioContent::Frame> ("AudioLength"))
{
	
}

string
SndfileContent::summary () const
{
	/* Get the string() here so that the name does not have quotes around it */
	return String::compose (_("%1 [audio]"), path_summary ());
}

string
SndfileContent::technical_summary () const
{
	return Content::technical_summary() + " - "
		+ AudioContent::technical_summary ()
		+ N_(" - sndfile");
}

bool
SndfileContent::valid_file (boost::filesystem::path f)
{
	/* XXX: more extensions */
	string ext = f.extension().string();
	transform (ext.begin(), ext.end(), ext.begin(), ::tolower);
	return (ext == ".wav" || ext == ".aif" || ext == ".aiff");
}

void
SndfileContent::examine (shared_ptr<Job> job)
{
	job->set_progress_unknown ();
	Content::examine (job);

	shared_ptr<const Film> film = _film.lock ();
	DCPOMATIC_ASSERT (film);

	SndfileDecoder dec (film, shared_from_this());

	{
		boost::mutex::scoped_lock lm (_mutex);
		_audio_length = dec.audio_length ();
		_audio_stream.reset (new AudioStream (dec.audio_length (), dec.audio_channels ()));
	}
	
	signal_changed (AudioContentProperty::AUDIO_CHANNELS);
	signal_changed (AudioContentProperty::AUDIO_LENGTH);
	signal_changed (AudioContentProperty::AUDIO_FRAME_RATE);
	signal_changed (AudioContentProperty::AUDIO_MAPPING);
}

void
SndfileContent::as_xml (xmlpp::Node* node) const
{
	node->add_child("Type")->add_child_text ("Sndfile");
	Content::as_xml (node);
	AudioContent::as_xml (node);

	node->add_child("AudioChannels")->add_child_text (raw_convert<string> (audio_channels ()));
	node->add_child("AudioLength")->add_child_text (raw_convert<string> (audio_length ()));
	node->add_child("AudioFrameRate")->add_child_text (raw_convert<string> (audio_stream()->frame_rate ()));
	audio_mapping().as_xml (node->add_child("AudioMapping"));
}

Time
SndfileContent::full_length () const
{
	shared_ptr<const Film> film = _film.lock ();
	DCPOMATIC_ASSERT (film);

	OutputAudioFrame const len = divide_with_round (
		audio_length() * output_audio_frame_rate(), audio_stream()->frame_rate()
		);
	
	return film->audio_frames_to_time (len);
}

AudioContent::Frame
SndfileContent::audio_length () const
{
	boost::mutex::scoped_lock lm (_mutex);
	return _audio_length;
}

vector<AudioStreamPtr>
SndfileContent::audio_streams () const
{
	vector<AudioStreamPtr> s;
	s.push_back (audio_stream ());
	return s;
}
