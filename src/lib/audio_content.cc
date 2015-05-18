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

#include <boost/foreach.hpp>
#include <libcxml/cxml.h>
#include "raw_convert.h"
#include "audio_content.h"
#include "analyse_audio_job.h"
#include "job_manager.h"
#include "film.h"
#include "exceptions.h"
#include "config.h"
#include "frame_rate_change.h"

#include "i18n.h"

using std::string;
using std::cout;
using std::vector;
using std::fixed;
using std::setprecision;
using std::stringstream;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using boost::optional;

int const AudioContentProperty::AUDIO_CHANNELS = 200;
int const AudioContentProperty::AUDIO_LENGTH = 201;
int const AudioContentProperty::AUDIO_FRAME_RATE = 202;
int const AudioContentProperty::AUDIO_GAIN = 203;
int const AudioContentProperty::AUDIO_DELAY = 204;
int const AudioContentProperty::AUDIO_MAPPING = 205;

AudioContent::AudioContent (shared_ptr<const Film> f, Time s)
	: Content (f, s)
	, _audio_gain (0)
	, _audio_delay (Config::instance()->default_audio_delay ())
{

}

AudioContent::AudioContent (shared_ptr<const Film> f, boost::filesystem::path p)
	: Content (f, p)
	, _audio_gain (0)
	, _audio_delay (Config::instance()->default_audio_delay ())
{

}

AudioContent::AudioContent (shared_ptr<const Film> f, shared_ptr<const cxml::Node> node)
	: Content (f, node)
{
	_audio_gain = node->number_child<float> ("AudioGain");
	_audio_delay = node->number_child<int> ("AudioDelay");
}

AudioContent::AudioContent (shared_ptr<const Film> f, vector<shared_ptr<Content> > c)
	: Content (f, c)
{
	shared_ptr<AudioContent> ref = dynamic_pointer_cast<AudioContent> (c[0]);
	DCPOMATIC_ASSERT (ref);
	
	for (size_t i = 0; i < c.size(); ++i) {
		shared_ptr<AudioContent> ac = dynamic_pointer_cast<AudioContent> (c[i]);

		if (ac->audio_gain() != ref->audio_gain()) {
			throw JoinError (_("Content to be joined must have the same audio gain."));
		}

		if (ac->audio_delay() != ref->audio_delay()) {
			throw JoinError (_("Content to be joined must have the same audio delay."));
		}
	}

	_audio_gain = ref->audio_gain ();
	_audio_delay = ref->audio_delay ();
}

void
AudioContent::as_xml (xmlpp::Node* node) const
{
	boost::mutex::scoped_lock lm (_mutex);
	node->add_child("AudioGain")->add_child_text (raw_convert<string> (_audio_gain));
	node->add_child("AudioDelay")->add_child_text (raw_convert<string> (_audio_delay));
}


void
AudioContent::set_audio_gain (double g)
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

boost::signals2::connection
AudioContent::analyse_audio (boost::function<void()> finished)
{
	shared_ptr<const Film> film = _film.lock ();
	DCPOMATIC_ASSERT (film);
	
	shared_ptr<AnalyseAudioJob> job (new AnalyseAudioJob (film, dynamic_pointer_cast<AudioContent> (shared_from_this())));
	boost::signals2::connection c = job->Finished.connect (finished);
	JobManager::instance()->add (job);

	return c;
}

boost::filesystem::path
AudioContent::audio_analysis_path () const
{
	shared_ptr<const Film> film = _film.lock ();
	if (!film) {
		return boost::filesystem::path ();
	}

	boost::filesystem::path p = film->audio_analysis_dir ();
	p /= digest() + "_" + audio_mapping().digest();
	return p;
}

string
AudioContent::technical_summary () const
{
	return String::compose (
		N_("audio: channels %1, length %2, out rate %3"),
		audio_channels(), audio_length(), output_audio_frame_rate()
		);
}

int
AudioContent::output_audio_frame_rate () const
{
	shared_ptr<const Film> film = _film.lock ();
	DCPOMATIC_ASSERT (film);
	
	/* Resample to a DCI-approved sample rate */
	double t = has_rate_above_48k() ? 96000 : 48000;

	FrameRateChange frc = film->active_frame_rate_change (position ());

	/* Compensate if the DCP is being run at a different frame rate
	   to the source; that is, if the video is run such that it will
	   look different in the DCP compared to the source (slower or faster).
	   skip/repeat doesn't come into effect here.
	*/

	if (frc.change_speed) {
		t /= frc.speed_up;
	}

	return rint (t);
}

string
AudioContent::processing_description () const
{
	vector<AudioStreamPtr> streams = audio_streams ();
	if (streams.empty ()) {
		return "";
	}

	/* Possible answers are:
	   1. all audio will be resampled from x to y.
	   2. all audio will be resampled to y (from a variety of rates)
	   3. some audio will be resampled to y (from a variety of rates)
	   4. nothing will be resampled.
	*/

	bool not_resampled = false;
	bool resampled = false;
	bool same = true;

	optional<int> common_frame_rate;
	BOOST_FOREACH (AudioStreamPtr i, streams) {
		if (i->frame_rate() != output_audio_frame_rate()) {
			resampled = true;
		} else {
			not_resampled = true;
		}

		if (common_frame_rate && common_frame_rate != i->frame_rate ()) {
			same = false;
		}
		common_frame_rate = i->frame_rate ();
	}

	if (not_resampled && !resampled) {
		return _("Audio will not be resampled");
	}

	if (not_resampled && resampled) {
		return String::compose (_("Some audio will be resampled to %1kHz"), output_audio_frame_rate ());
	}

	if (!not_resampled && resampled) {
		if (same) {
			return String::compose (_("Audio will be resampled from %1kHz to %2kHz"), common_frame_rate.get(), output_audio_frame_rate ());
		} else {
			return String::compose (_("Audio will be resampled to %1kHz"), output_audio_frame_rate ());
		}
	}

	return "";
}

bool
AudioContent::has_rate_above_48k () const
{
	BOOST_FOREACH (AudioStreamPtr i, audio_streams ()) {
		if (i->frame_rate() > 48000) {
			return true;
		}
	}

	return false;
}


void
AudioContent::set_audio_mapping (AudioMapping mapping)
{
	int c = 0;
	BOOST_FOREACH (AudioStreamPtr i, audio_streams ()) {
		AudioMapping stream_mapping (i->channels ());
		for (int j = 0; j < i->channels(); ++j) {
			for (int k = 0; k < MAX_DCP_AUDIO_CHANNELS; ++k) {
				stream_mapping.set (j, static_cast<libdcp::Channel> (k), mapping.get (c, static_cast<libdcp::Channel> (k)));
			}
			++c;
		}
		i->set_mapping (stream_mapping);
	}
		
	signal_changed (AudioContentProperty::AUDIO_MAPPING);
}


AudioMapping
AudioContent::audio_mapping () const
{
	AudioMapping merged (audio_channels ());
	
	int c = 0;
	int s = 0;
	BOOST_FOREACH (AudioStreamPtr i, audio_streams ()) {
		AudioMapping mapping = i->mapping ();
		for (int j = 0; j < mapping.content_channels(); ++j) {
			merged.set_name (c, String::compose ("%1:%2", s + 1, j + 1));
			for (int k = 0; k < MAX_DCP_AUDIO_CHANNELS; ++k) {
				merged.set (c, static_cast<libdcp::Channel> (k), mapping.get (j, static_cast<libdcp::Channel> (k)));
			}
			++c;
		}
		++s;
	}

	return merged;
}

int
AudioContent::audio_channels () const
{
	int channels = 0;
	BOOST_FOREACH (AudioStreamPtr i, audio_streams ()) {
		channels += i->channels ();
	}

	return channels;
}

