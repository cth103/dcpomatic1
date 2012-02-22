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
#include "j2k_wav_encoder.h"
#include "config.h"
#include "film_state.h"
#include "options.h"
#include "exceptions.h"
#include "dcp_video_frame.h"
#include "server.h"

using namespace std;
using namespace boost;

J2KWAVEncoder::J2KWAVEncoder (shared_ptr<const FilmState> s, shared_ptr<const Options> o)
	: Encoder (s, o)
	, _deinterleave_buffer_size (8192)
	, _deinterleave_buffer (0)
	, _process_end (false)
{
	/* Create sound output files with .tmp suffixes; we will rename
	   them if and when we complete.
	*/
	for (int i = 0; i < _fs->audio_channels; ++i) {
		SF_INFO sf_info;
		sf_info.samplerate = _fs->audio_sample_rate;
		/* We write mono files */
		sf_info.channels = 1;
		sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
		SNDFILE* f = sf_open (_opt->multichannel_audio_out_path (i, true).c_str (), SFM_WRITE, &sf_info);
		if (f == 0) {
			throw CreateFileError (_opt->multichannel_audio_out_path (i, true));
		}
		_sound_files.push_back (f);
	}

	/* Create buffer for deinterleaving audio */
	_deinterleave_buffer = new uint8_t[_deinterleave_buffer_size];
}

J2KWAVEncoder::~J2KWAVEncoder ()
{
	delete[] _deinterleave_buffer;

	for (vector<SNDFILE*>::iterator i = _sound_files.begin(); i != _sound_files.end(); ++i) {
		sf_close (*i);
	}
}	

void
J2KWAVEncoder::process_video (shared_ptr<Image> yuv, int frame)
{
	boost::mutex::scoped_lock lock (_worker_mutex);

	/* Wait until the queue has gone down a bit */
	while (_queue.size() >= _worker_threads.size() * 2 && !_process_end) {
		_worker_condition.wait (lock);
	}

	if (_process_end) {
		return;
	}

	/* Only do the processing if we don't already have a file for this frame */
	if (!boost::filesystem::exists (_opt->frame_out_path (frame, false))) {
		_queue.push_back (boost::shared_ptr<DCPVideoFrame> (
					  new DCPVideoFrame (yuv, Size (_opt->out_width, _opt->out_height), _fs->scaler, frame, _fs->frames_per_second)
					  ));
		
		_worker_condition.notify_all ();
	}
}

void
J2KWAVEncoder::encoder_thread (Server* server)
{
	int failures = 0;
	
	while (1) {
		boost::mutex::scoped_lock lock (_worker_mutex);
		while (_queue.empty () && !_process_end) {
			_worker_condition.wait (lock);
		}

		if (_process_end) {
			return;
		}

		boost::shared_ptr<DCPVideoFrame> vf = _queue.front ();
		_queue.pop_front ();
		
		lock.unlock ();

		if (server) {
			try {
				vf->encode_remotely (server);
			} catch (std::exception& e) {
				++failures;
				cerr << "Remote encode failed (" << e.what() << "); thread sleeping for " << failures << "s.\n";
				sleep (failures);
			}
				
		} else {
			try {
				vf->encode_locally ();
			} catch (std::exception& e) {
				cerr << "Local encode failed " << e.what() << ".\n";
			}
		}

		if (vf->encoded ()) {
			vf->encoded()->write (_opt, vf->frame ());
		} else {
			lock.lock ();
			_queue.push_front (vf);
			lock.unlock ();
		}

		if (failures == 4) {
			cerr << "Giving up on encode thread.\n";
			return;
		}

		lock.lock ();
		_worker_condition.notify_all ();
	}
}

void
J2KWAVEncoder::process_begin ()
{
	for (int i = 0; i < Config::instance()->num_local_encoding_threads (); ++i) {
		_worker_threads.push_back (new boost::thread (boost::bind (&J2KWAVEncoder::encoder_thread, this, (Server *) 0)));
	}

	list<Server*> servers = Config::instance()->servers ();

	for (list<Server*>::iterator i = servers.begin(); i != servers.end(); ++i) {
		for (int j = 0; j < (*i)->threads (); ++j) {
			_worker_threads.push_back (new boost::thread (boost::bind (&J2KWAVEncoder::encoder_thread, this, *i)));
		}
	}
}

void
J2KWAVEncoder::process_end ()
{
	boost::mutex::scoped_lock lock (_worker_mutex);

	/* Keep waking workers until the queue is empty */
	while (!_queue.empty ()) {
		_worker_condition.notify_all ();
		_worker_condition.wait (lock);
	}
	
	_process_end = true;
	_worker_condition.notify_all ();
	lock.unlock ();

	for (list<boost::thread *>::iterator i = _worker_threads.begin(); i != _worker_threads.end(); ++i) {
		(*i)->join ();
		delete *i;
	}

	/* Rename .wav.tmp files to .wav */
	for (int i = 0; i < _fs->audio_channels; ++i) {
		if (boost::filesystem::exists (_opt->multichannel_audio_out_path (i, false))) {
			boost::filesystem::remove (_opt->multichannel_audio_out_path (i, false));
		}
		boost::filesystem::rename (_opt->multichannel_audio_out_path (i, true), _opt->multichannel_audio_out_path (i, false));
	}
}

void
J2KWAVEncoder::process_audio (uint8_t* data, int channels, int data_size)
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
			
			switch (_fs->audio_sample_format) {
			case AV_SAMPLE_FMT_S16:
				sf_write_short (_sound_files[i], (const short *) _deinterleave_buffer, this_time / sample_size);
				break;
			default:
				throw DecodeError ("unknown audio sample format");
			}
		}
		
		position += this_time;
		remaining -= this_time * channels;
	}
}

