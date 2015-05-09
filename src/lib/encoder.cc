/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

/** @file src/encoder.h
 *  @brief Parent class for classes which can encode video and audio frames.
 */

#include <iostream>
#include <boost/lambda/lambda.hpp>
#include <libcxml/cxml.h>
#include "encoder.h"
#include "util.h"
#include "film.h"
#include "log.h"
#include "config.h"
#include "dcp_video_frame.h"
#include "server.h"
#include "cross.h"
#include "writer.h"
#include "server_finder.h"
#include "player.h"
#include "player_video_frame.h"

#include "i18n.h"

#define LOG_GENERAL(...) _film->log()->log (String::compose (__VA_ARGS__), Log::TYPE_GENERAL);
#define LOG_GENERAL_NC(...) _film->log()->log (__VA_ARGS__, Log::TYPE_GENERAL);
#define LOG_ERROR(...) _film->log()->log (String::compose (__VA_ARGS__), Log::TYPE_ERROR);
#define LOG_TIMING(...) _film->log()->microsecond_log (String::compose (__VA_ARGS__), Log::TYPE_TIMING);
#define LOG_TIMING_NC(...) _film->log()->microsecond_log (__VA_ARGS__, Log::TYPE_TIMING);
#define LOG_DEBUG_NC(...) _film->log()->microsecond_log (__VA_ARGS__, Log::TYPE_DEBUG);

using std::pair;
using std::string;
using std::vector;
using std::list;
using std::cout;
using std::min;
using std::make_pair;
using boost::shared_ptr;
using boost::weak_ptr;
using boost::optional;
using boost::scoped_array;

int const Encoder::_history_size = 25;

/** @param f Film that we are encoding */
Encoder::Encoder (shared_ptr<const Film> f, weak_ptr<Job> j)
	: _film (f)
	, _job (j)
	, _video_frames_out (0)
	, _terminate (false)
{
	_have_a_real_frame[EYES_BOTH] = false;
	_have_a_real_frame[EYES_LEFT] = false;
	_have_a_real_frame[EYES_RIGHT] = false;
}

Encoder::~Encoder ()
{
	terminate_threads ();
}

/** Add a worker thread for a each thread on a remote server.  Caller must hold
 *  a lock on _mutex, or know that one is not currently required to
 *  safely modify _threads.
 */
void
Encoder::add_worker_threads (ServerDescription d)
{
	LOG_GENERAL (N_("Adding %1 worker threads for remote %2"), d.threads(), d.host_name ());
	for (int i = 0; i < d.threads(); ++i) {
		_threads.push_back (new boost::thread (boost::bind (&Encoder::encoder_thread, this, d)));
	}
}

void
Encoder::process_begin ()
{
	for (int i = 0; i < Config::instance()->num_local_encoding_threads (); ++i) {
		_threads.push_back (new boost::thread (boost::bind (&Encoder::encoder_thread, this, optional<ServerDescription> ())));
	}

	_writer.reset (new Writer (_film, _job));
	if (!ServerFinder::instance()->disabled ()) {
		_server_found_connection = ServerFinder::instance()->connect (boost::bind (&Encoder::server_found, this, _1));
	}
}

void
Encoder::process_end ()
{
	boost::mutex::scoped_lock lock (_mutex);

	LOG_GENERAL (N_("Clearing queue of %1"), _queue.size ());

	/* Keep waking workers until the queue is empty */
	while (!_queue.empty ()) {
		_empty_condition.notify_all ();
		_full_condition.wait (lock);
	}

	lock.unlock ();
	
	terminate_threads ();

	LOG_GENERAL (N_("Mopping up %1"), _queue.size());

	/* The following sequence of events can occur in the above code:
	     1. a remote worker takes the last image off the queue
	     2. the loop above terminates
	     3. the remote worker fails to encode the image and puts it back on the queue
	     4. the remote worker is then terminated by terminate_threads

	     So just mop up anything left in the queue here.
	*/

	for (list<shared_ptr<DCPVideoFrame> >::iterator i = _queue.begin(); i != _queue.end(); ++i) {
		LOG_GENERAL (N_("Encode left-over frame %1"), (*i)->index ());
		try {
			_writer->write ((*i)->encode_locally(), (*i)->index (), (*i)->eyes ());
			frame_done ();
		} catch (std::exception& e) {
			LOG_ERROR (N_("Local encode failed (%1)"), e.what ());
		}
	}
		
	_writer->finish ();
	_writer.reset ();

	LOG_GENERAL_NC (N_("Encoder::process_end finished"));
}	

/** @return an estimate of the current number of frames we are encoding per second,
 *  or 0 if not known.
 */
float
Encoder::current_encoding_rate () const
{
	boost::mutex::scoped_lock lock (_state_mutex);
	if (int (_time_history.size()) < _history_size) {
		return 0;
	}

	struct timeval now;
	gettimeofday (&now, 0);

	return _history_size / (seconds (now) - seconds (_time_history.back ()));
}

/** @return Number of video frames that have been sent out */
int
Encoder::video_frames_out () const
{
	boost::mutex::scoped_lock (_state_mutex);
	return _video_frames_out;
}

/** Should be called when a frame has been encoded successfully.
 *  @param n Source frame index.
 */
void
Encoder::frame_done ()
{
	boost::mutex::scoped_lock lock (_state_mutex);
	
	struct timeval tv;
	gettimeofday (&tv, 0);
	_time_history.push_front (tv);
	if (int (_time_history.size()) > _history_size) {
		_time_history.pop_back ();
	}
}

void
Encoder::process_video (shared_ptr<PlayerVideoFrame> pvf, bool same)
{
	LOG_DEBUG_NC ("-> Encoder::process_video");
	
	_waker.nudge ();

	boost::mutex::scoped_lock lock (_mutex);

	/* XXX: discard 3D here if required */

	/* Wait until the queue has gone down a bit */
	while (_queue.size() >= _threads.size() * 2 && !_terminate) {
		LOG_TIMING ("decoder sleeps with queue of %1", _queue.size());
		_full_condition.wait (lock);
		LOG_TIMING ("decoder wakes with queue of %1", _queue.size());
	}

	if (_terminate) {
		LOG_DEBUG_NC ("<- Encoder::process_video terminated");
		return;
	}

	_writer->rethrow ();
	/* Re-throw any exception raised by one of our threads.  If more
	   than one has thrown an exception, only one will be rethrown, I think;
	   but then, if that happens something has gone badly wrong.
	*/
	rethrow ();

	if (_writer->can_fake_write (_video_frames_out)) {
		_writer->fake_write (_video_frames_out, pvf->eyes ());
		_have_a_real_frame[pvf->eyes()] = false;
		frame_done ();
	} else if (same && _have_a_real_frame[pvf->eyes()]) {
		/* Use the last frame that we encoded. */
		_writer->repeat (_video_frames_out, pvf->eyes());
		frame_done ();
	} else {
		/* Queue this new frame for encoding */
		LOG_TIMING ("adding to queue of %1", _queue.size ());
		_queue.push_back (shared_ptr<DCPVideoFrame> (
					  new DCPVideoFrame (
						  pvf, _video_frames_out, _film->video_frame_rate(),
						  _film->j2k_bandwidth(), _film->resolution(), _film->log()
						  )
					  ));

		/* The queue might not be empty any more, so notify anything which is
		   waiting on that.
		*/
		_empty_condition.notify_all ();
		_have_a_real_frame[pvf->eyes()] = true;
	}

	if (pvf->eyes() != EYES_LEFT) {
		++_video_frames_out;
	}

	LOG_DEBUG_NC ("<- Encoder::process_video");
}

void
Encoder::process_audio (shared_ptr<const AudioBuffers> data)
{
	_writer->write (data);
}

void
Encoder::terminate_threads ()
{
	{
		boost::mutex::scoped_lock lock (_mutex);
		_terminate = true;
		_full_condition.notify_all ();
		_empty_condition.notify_all ();
	}

	for (list<boost::thread *>::iterator i = _threads.begin(); i != _threads.end(); ++i) {
		if ((*i)->joinable ()) {
			(*i)->join ();
		}
		delete *i;
	}

	_threads.clear ();
}

void
Encoder::encoder_thread (optional<ServerDescription> server)
try
{
	/* Number of seconds that we currently wait between attempts
	   to connect to the server; not relevant for localhost
	   encodings.
	*/
	int remote_backoff = 0;
	
	while (true) {

		LOG_TIMING ("[%1] encoder thread sleeps", boost::this_thread::get_id());
		boost::mutex::scoped_lock lock (_mutex);
		while (_queue.empty () && !_terminate) {
			_empty_condition.wait (lock);
		}

		if (_terminate) {
			LOG_TIMING ("[%1] encoder thread terminates", boost::this_thread::get_id());
			return;
		}

		LOG_TIMING ("[%1] encoder thread wakes with queue of %2", boost::this_thread::get_id(), _queue.size());
		shared_ptr<DCPVideoFrame> vf = _queue.front ();
		LOG_TIMING ("[%1] encoder thread pops frame %2 (%3) from queue", boost::this_thread::get_id(), vf->index(), vf->eyes ());
		_queue.pop_front ();
		
		lock.unlock ();

		shared_ptr<EncodedData> encoded;

		if (server) {
			try {
				encoded = vf->encode_remotely (server.get ());

				if (remote_backoff > 0) {
					LOG_GENERAL ("%1 was lost, but now she is found; removing backoff", server->host_name ());
				}
				
				/* This job succeeded, so remove any backoff */
				remote_backoff = 0;
				
			} catch (std::exception& e) {
				if (remote_backoff < 60) {
					/* back off more */
					remote_backoff += 10;
				}
				LOG_ERROR (
					N_("Remote encode of %1 on %2 failed (%3); thread sleeping for %4s"),
					vf->index(), server->host_name(), e.what(), remote_backoff
					);
			}
				
		} else {
			try {
				LOG_TIMING ("[%1] encoder thread begins local encode of %2", boost::this_thread::get_id(), vf->index());
				encoded = vf->encode_locally ();
				LOG_TIMING ("[%1] encoder thread finishes local encode of %2", boost::this_thread::get_id(), vf->index());
			} catch (std::exception& e) {
				LOG_ERROR (N_("Local encode failed (%1)"), e.what ());
			}
		}

		if (encoded) {
			_writer->write (encoded, vf->index (), vf->eyes ());
			frame_done ();
		} else {
			lock.lock ();
			LOG_GENERAL (N_("[%1] Encoder thread pushes frame %2 back onto queue after failure"), boost::this_thread::get_id(), vf->index());
			_queue.push_front (vf);
			lock.unlock ();
		}

		if (remote_backoff > 0) {
			dcpomatic_sleep (remote_backoff);
		}

		/* The queue might not be full any more, so notify anything that is waiting on that */
		lock.lock ();
		_full_condition.notify_all ();
	}
}
catch (...)
{
	store_current ();
}

void
Encoder::server_found (ServerDescription s)
{
	add_worker_threads (s);
}
