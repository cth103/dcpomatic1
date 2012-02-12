/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <iostream>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <sndfile.h>
#include <openjpeg.h>
#include "j2k_wav_transcoder.h"
#include "film.h"
#include "config.h"

using namespace std;

J2KWAVTranscoder::J2KWAVTranscoder (Film* f, Progress* p, int w, int h)
	: Transcoder (f, p, w, h)
	, _deinterleave_buffer_size (8192)
	, _deinterleave_buffer (0)
	, _process_end (false)
	, _num_worker_threads (Config::instance()->num_encoding_threads ())
{
	/* Create sound output files with .tmp suffixes; we will rename
	   them if and when we complete.
	*/
	for (int i = 0; i < audio_channels(); ++i) {
		SF_INFO sf_info;
		sf_info.samplerate = audio_sample_rate();
		/* We write mono files */
		sf_info.channels = 1;
		sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
		SNDFILE* f = sf_open (wav_path(i, true).c_str(), SFM_WRITE, &sf_info);
		if (f == 0) {
			throw runtime_error ("Could not create audio output file");
		}
		_sound_files.push_back (f);
	}

	/* Create buffer for deinterleaving audio */
	_deinterleave_buffer = new uint8_t[_deinterleave_buffer_size];
}

J2KWAVTranscoder::~J2KWAVTranscoder ()
{
	delete[] _deinterleave_buffer;

	for (vector<SNDFILE*>::iterator i = _sound_files.begin(); i != _sound_files.end(); ++i) {
		sf_close (*i);
	}
}	

void
J2KWAVTranscoder::process_video (uint8_t* rgb, int line_size)
{
	boost::mutex::scoped_lock lock (_worker_mutex);

	/* Wait until the queue has gone down a bit */
	while (int (_queue.size()) >= _num_worker_threads * 2 && !_process_end) {
		_worker_condition.wait (lock);
	}

	if (_process_end) {
		return;
	}

	_queue.push_back (boost::shared_ptr<Image> (new Image (_film, rgb, out_width(), out_height(), video_frame())));
	_worker_condition.notify_all ();
}

void
J2KWAVTranscoder::encoder_thread ()
{
	while (1) {
		boost::mutex::scoped_lock lock (_worker_mutex);
		while (_queue.empty () && !_process_end) {
			_worker_condition.wait (_worker_mutex);
		}

		if (_process_end) {
			return;
		}

		boost::shared_ptr<Image> im = _queue.front ();
		_queue.pop_front ();
		
		lock.unlock ();
		im->encode ();
		lock.lock ();

		_worker_condition.notify_all ();
	}
}

void
J2KWAVTranscoder::process_begin ()
{
	for (int i = 0; i < _num_worker_threads; ++i) {
		_worker_threads.push_back (new boost::thread (boost::bind (&J2KWAVTranscoder::encoder_thread, this)));
	}
}

void
J2KWAVTranscoder::process_end ()
{
	boost::mutex::scoped_lock lock (_worker_mutex);
	_process_end = true;
	lock.unlock ();
	
	_worker_condition.notify_all ();

	for (list<boost::thread *>::iterator i = _worker_threads.begin(); i != _worker_threads.end(); ++i) {
		(*i)->join ();
		delete *i;
	}

	/* Rename .wav.tmp files to .wav */
	for (int i = 0; i < audio_channels(); ++i) {
		boost::filesystem::rename (wav_path (i, true), wav_path (i, false));
	}
}

/** @param i 0-based index */
string
J2KWAVTranscoder::wav_path (int i, bool tmp) const
{
	stringstream s;
	s << _film->dir ("wavs") << "/" << (i + 1) << ".wav";
	if (tmp) {
		s << ".tmp";
	}

	return s.str ();
}

void
J2KWAVTranscoder::process_audio (uint8_t* data, int channels, int data_size)
{
	/* Size of a sample in bytes */
	int const sample_size = 2;
	
	/* XXX: we are assuming that sample_size is right, the _deinterleave_buffer_size is a multiple
	   of the sample size and that data_size is a multiple of channels * sample_size.
	*/
	
	/* XXX: this code is very tricksy and it must be possible to make it simpler ... */
	
	/* Number of bytes left to read this time */
	int remaining = data_size;
	/* Our position in the output buffers, in bytes */
	int position = 0;
	while (remaining > 0) {
		/* How many bytes of the deinterleaved data to do this time */
		int this_time = min (remaining / channels, _deinterleave_buffer_size);
		for (int i = 0; i < channels; ++i) {
			for (int j = 0; j < this_time; j += sample_size) {
				for (int k = 0; k < sample_size; ++k) {
					int const to = j + k;
					int const from = position + (i * sample_size) + (j * channels) + k;
					_deinterleave_buffer[to] = data[from];
				}
			}
			
			switch (audio_sample_format ()) {
			case AV_SAMPLE_FMT_S16:
				sf_write_short (_sound_files[i], (const short *) _deinterleave_buffer, this_time / sample_size);
				break;
			default:
				throw runtime_error ("Unknown audio sample format");
			}
		}
		
		position += this_time;
		remaining -= this_time * channels;
	}
}

