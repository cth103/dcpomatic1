/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

#include "audio_analysis.h"
#include "analyse_audio_job.h"
#include "compose.hpp"
#include "film.h"
#include "player.h"
#include "log.h"
#include "compose.hpp"

#include "i18n.h"

using std::string;
using std::max;
using std::min;
using std::cout;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;

#define LOG_DEBUG_NC(...) _film->log()->log (__VA_ARGS__, Log::TYPE_DEBUG);
#define LOG_DEBUG(...) _film->log()->log (String::compose (__VA_ARGS__), Log::TYPE_DEBUG);
#define LOG_TIMING(...) _film->log()->microsecond_log (String::compose (__VA_ARGS__), Log::TYPE_TIMING);

int const AnalyseAudioJob::_num_points = 1024;

AnalyseAudioJob::AnalyseAudioJob (shared_ptr<const Film> f, shared_ptr<AudioContent> c)
	: Job (f)
	, _content (dynamic_pointer_cast<AudioContent> (c->clone ()))
	, _done (0)
	, _samples_per_point (1)
	, _overall_peak (0)
	, _overall_peak_frame (0)
{

}

string
AnalyseAudioJob::name () const
{
	return _("Analyse audio");
}

string
AnalyseAudioJob::json_name () const
{
	return N_("analyse_audio");
}

void
AnalyseAudioJob::run ()
{
	shared_ptr<Playlist> playlist (new Playlist);
	_content->set_position (0);
	playlist->add (_content);
	shared_ptr<Player> player (new Player (_film, playlist));
	player->disable_video ();

	_player_connection = player->Audio.connect (bind (&AnalyseAudioJob::audio, this, _1, _2));

	_samples_per_point = max (int64_t (1), _film->time_to_audio_frames (_film->length()) / _num_points);

	_current.resize (_film->audio_channels ());
	_analysis.reset (new AudioAnalysis (_film->audio_channels ()));

	_done = 0;
	OutputAudioFrame const len = _film->time_to_audio_frames (_film->length ());
	while (!player->pass ()) {
		set_progress (double (_done) / len);
	}

	_analysis->set_peak (_overall_peak, _film->audio_frames_to_time (_overall_peak_frame));
	_analysis->write (_content->audio_analysis_path ());

	set_progress (1);
	set_state (FINISHED_OK);
}

void
AnalyseAudioJob::audio (shared_ptr<const AudioBuffers> b, Time)
{
	for (int i = 0; i < b->frames(); ++i) {
		for (int j = 0; j < b->channels(); ++j) {
			float s = b->data(j)[i];
			if (fabsf (s) < 10e-7) {
				/* SafeStringStream can't serialise and recover inf or -inf, so prevent such
				   values by replacing with this (140dB down) */
				s = 10e-7;
			}
			_current[j][AudioPoint::RMS] += pow (s, 2);

			float const as = fabsf (s);

			_current[j][AudioPoint::PEAK] = max (_current[j][AudioPoint::PEAK], as);

			if (as > _overall_peak) {
				_overall_peak = as;
				_overall_peak_frame = _done + i;
			}

			if ((_done % _samples_per_point) == 0) {
				_current[j][AudioPoint::RMS] = sqrt (_current[j][AudioPoint::RMS] / _samples_per_point);
				_analysis->add_point (j, _current[j]);

				_current[j] = AudioPoint ();
			}
		}

		++_done;
	}
}
