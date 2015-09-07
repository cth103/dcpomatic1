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

#include "job.h"
#include "audio_analysis.h"
#include "types.h"

class AudioBuffers;
class AudioContent;

class AnalyseAudioJob : public Job
{
public:
	AnalyseAudioJob (boost::shared_ptr<const Film>, boost::shared_ptr<AudioContent>);

	std::string name () const;
	std::string json_name () const;
	void run ();

private:
	void audio (boost::shared_ptr<const AudioBuffers>, Time);

	boost::shared_ptr<AudioContent> _content;
	OutputAudioFrame _done;
	int64_t _samples_per_point;
	std::vector<AudioPoint> _current;
	float _overall_peak;
	OutputAudioFrame _overall_peak_frame;

	boost::shared_ptr<AudioAnalysis> _analysis;

	boost::signals2::scoped_connection _player_connection;

	static const int _num_points;
};
