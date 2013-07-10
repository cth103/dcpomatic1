/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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
#include "audio_content.h"
#include "analyse_audio_job.h"
#include "job_manager.h"
#include "film.h"

using std::string;
using boost::shared_ptr;
using boost::lexical_cast;
using boost::dynamic_pointer_cast;

int const AudioContentProperty::AUDIO_CHANNELS = 200;
int const AudioContentProperty::AUDIO_LENGTH = 201;
int const AudioContentProperty::AUDIO_FRAME_RATE = 202;
int const AudioContentProperty::AUDIO_GAIN = 203;
int const AudioContentProperty::AUDIO_DELAY = 204;
int const AudioContentProperty::AUDIO_MAPPING = 205;

AudioContent::AudioContent (shared_ptr<const Film> f, Time s)
	: Content (f, s)
	, _audio_gain (0)
	, _audio_delay (0)
{

}

AudioContent::AudioContent (shared_ptr<const Film> f, boost::filesystem::path p)
	: Content (f, p)
	, _audio_gain (0)
	, _audio_delay (0)
{

}

AudioContent::AudioContent (shared_ptr<const Film> f, shared_ptr<const cxml::Node> node)
	: Content (f, node)
{
	_audio_gain = node->number_child<float> ("AudioGain");
	_audio_delay = node->number_child<int> ("AudioDelay");
}

AudioContent::AudioContent (AudioContent const & o)
	: Content (o)
	, _audio_gain (o._audio_gain)
	, _audio_delay (o._audio_delay)
{

}

void
AudioContent::as_xml (xmlpp::Node* node) const
{
	boost::mutex::scoped_lock lm (_mutex);
	node->add_child("AudioGain")->add_child_text (lexical_cast<string> (_audio_gain));
	node->add_child("AudioDelay")->add_child_text (lexical_cast<string> (_audio_delay));
}


void
AudioContent::set_audio_gain (float g)
{
	{
		boost::mutex::scoped_lock lm (_mutex);
		_audio_gain = g;
	}
	
	signal_changed (AudioContentProperty::AUDIO_GAIN);
}

void
AudioContent::set_audio_delay (int d)
{
	{
		boost::mutex::scoped_lock lm (_mutex);
		_audio_delay = d;
	}
	
	signal_changed (AudioContentProperty::AUDIO_DELAY);
}

void
AudioContent::analyse_audio (boost::function<void()> finished)
{
	shared_ptr<const Film> film = _film.lock ();
	if (!film) {
		return;
	}
	
	shared_ptr<AnalyseAudioJob> job (new AnalyseAudioJob (film, dynamic_pointer_cast<AudioContent> (shared_from_this())));
	job->Finished.connect (finished);
	JobManager::instance()->add (job);
}

boost::filesystem::path
AudioContent::audio_analysis_path () const
{
	shared_ptr<const Film> film = _film.lock ();
	if (!film) {
		return boost::filesystem::path ();
	}

	return film->audio_analysis_path (dynamic_pointer_cast<const AudioContent> (shared_from_this ()));
}