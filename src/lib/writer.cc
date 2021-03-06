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

#include <fstream>
#include <cerrno>
#include <libdcp/mono_picture_asset.h>
#include <libdcp/stereo_picture_asset.h>
#include <libdcp/sound_asset.h>
#include <libdcp/reel.h>
#include <libdcp/dcp.h>
#include <libdcp/cpl.h>
#include "writer.h"
#include "compose.hpp"
#include "film.h"
#include "ratio.h"
#include "log.h"
#include "dcp_video_frame.h"
#include "dcp_content_type.h"
#include "player.h"
#include "audio_mapping.h"
#include "config.h"
#include "job.h"
#include "cross.h"
#include "md5_digester.h"
#include "version.h"

#include "i18n.h"

#define LOG_GENERAL(...) _film->log()->log (String::compose (__VA_ARGS__), Log::TYPE_GENERAL);
#define LOG_GENERAL_NC(...) _film->log()->log (__VA_ARGS__, Log::TYPE_GENERAL);
#define LOG_DEBUG(...) _film->log()->log (String::compose (__VA_ARGS__), Log::TYPE_DEBUG);
#define LOG_DEBUG_NC(...) _film->log()->log (__VA_ARGS__, Log::TYPE_DEBUG);
#define LOG_TIMING(...) _film->log()->microsecond_log (String::compose (__VA_ARGS__), Log::TYPE_TIMING);
#define LOG_WARNING(...) _film->log()->log (String::compose (__VA_ARGS__), Log::TYPE_WARNING);
#define LOG_WARNING_NC(...) _film->log()->log (__VA_ARGS__, Log::TYPE_WARNING);
#define LOG_ERROR(...) _film->log()->log (String::compose (__VA_ARGS__), Log::TYPE_ERROR);

/* OS X strikes again */
#undef set_key

using std::make_pair;
using std::pair;
using std::string;
using std::list;
using std::cout;
using boost::shared_ptr;
using boost::weak_ptr;

/** @param encoder_threads Total number of threads (local and remote) that the encoder is using */
Writer::Writer (shared_ptr<const Film> f, weak_ptr<Job> j)
	: _film (f)
	, _job (j)
	, _first_nonexistant_frame (0)
	, _thread (0)
	, _finish (false)
	, _queued_full_in_memory (0)
	, _last_written_frame (-1)
	, _last_written_eyes (EYES_RIGHT)
	, _maximum_frames_in_memory (0)
	, _full_written (0)
	, _fake_written (0)
	, _repeat_written (0)
	, _pushed_to_disk (0)
{
	/* Remove any old DCP */
	boost::filesystem::remove_all (_film->dir (_film->dcp_name ()));

	shared_ptr<Job> job = _job.lock ();
	DCPOMATIC_ASSERT (job);

	/* Create our picture asset in a subdirectory, named according to those
	   film's parameters which affect the video output.  We will hard-link
	   it into the DCP later.
	*/

	if (_film->three_d ()) {
		_picture_asset.reset (new libdcp::StereoPictureAsset (_film->internal_video_mxf_dir (), _film->internal_video_mxf_filename ()));
	} else {
		_picture_asset.reset (new libdcp::MonoPictureAsset (_film->internal_video_mxf_dir (), _film->internal_video_mxf_filename ()));
	}

	job->sub (_("Checking existing image data"));
	check_existing_picture_mxf ();

	_picture_asset->set_edit_rate (_film->video_frame_rate ());
	_picture_asset->set_size (_film->frame_size ());
	_picture_asset->set_interop (_film->interop ());

	if (_film->encrypted ()) {
		_picture_asset->set_key (_film->key ());
	}

	_picture_asset_writer = _picture_asset->start_write (_first_nonexistant_frame > 0);

	if (_film->audio_channels ()) {
		_sound_asset.reset (new libdcp::SoundAsset (_film->directory (), ""));
		_sound_asset->set_file_name (audio_mxf_filename (_sound_asset));
		_sound_asset->set_edit_rate (_film->video_frame_rate ());
		_sound_asset->set_channels (_film->audio_channels ());
		_sound_asset->set_sampling_rate (_film->audio_frame_rate ());
		_sound_asset->set_interop (_film->interop ());

		if (_film->encrypted ()) {
			_sound_asset->set_key (_film->key ());
		}

		/* Write the sound asset into the film directory so that we leave the creation
		   of the DCP directory until the last minute.
		*/
		_sound_asset_writer = _sound_asset->start_write ();
	}

	set_encoder_threads (Config::instance()->num_local_encoding_threads ());

	_thread = new boost::thread (boost::bind (&Writer::thread, this));

	job->sub (_("Encoding image data"));
}

Writer::~Writer ()
{
	terminate_thread (false);
}

void
Writer::write (shared_ptr<const EncodedData> encoded, int frame, Eyes eyes)
{
	boost::mutex::scoped_lock lock (_mutex);

	while (_queued_full_in_memory > _maximum_frames_in_memory) {
		/* The queue is too big; wait until that is sorted out */
		_full_condition.wait (lock);
	}

	QueueItem qi;
	qi.type = QueueItem::FULL;
	qi.encoded = encoded;
	qi.frame = frame;

	if (_film->three_d() && eyes == EYES_BOTH) {
		/* 2D material in a 3D DCP; fake the 3D */
		qi.eyes = EYES_LEFT;
		_queue.push_back (qi);
		++_queued_full_in_memory;
		qi.eyes = EYES_RIGHT;
		_queue.push_back (qi);
		++_queued_full_in_memory;
	} else {
		qi.eyes = eyes;
		_queue.push_back (qi);
		++_queued_full_in_memory;
	}

	/* Now there's something to do: wake anything wait()ing on _empty_condition */
	_empty_condition.notify_all ();
}

void
Writer::fake_write (int frame, Eyes eyes)
{
	boost::mutex::scoped_lock lock (_mutex);

	while (_queued_full_in_memory > _maximum_frames_in_memory) {
		/* The queue is too big; wait until that is sorted out */
		_full_condition.wait (lock);
	}

	FILE* file = fopen_boost (_film->info_file (), "rb");
	if (!file) {
		throw ReadFileError (_film->info_file ());
	}
	libdcp::FrameInfo info = read_frame_info (file, frame, eyes);
	fclose (file);

	QueueItem qi;
	qi.type = QueueItem::FAKE;
	qi.size = info.size;
	qi.frame = frame;
	if (_film->three_d() && eyes == EYES_BOTH) {
		qi.eyes = EYES_LEFT;
		_queue.push_back (qi);
		qi.eyes = EYES_RIGHT;
		_queue.push_back (qi);
	} else {
		qi.eyes = eyes;
		_queue.push_back (qi);
	}

	/* Now there's something to do: wake anything wait()ing on _empty_condition */
	_empty_condition.notify_all ();
}

/** This method is not thread safe */
void
Writer::write (shared_ptr<const AudioBuffers> audio)
{
	if (_sound_asset) {
		_sound_asset_writer->write (audio->data(), audio->frames());
	}
}

/** This must be called from Writer::thread() with an appropriate lock held */
bool
Writer::have_sequenced_image_at_queue_head ()
{
	if (_queue.empty ()) {
		return false;
	}

	_queue.sort ();

	/* The queue should contain only EYES_LEFT/EYES_RIGHT pairs or EYES_BOTH */

	if (_queue.front().eyes == EYES_BOTH) {
		/* 2D */
		return _queue.front().frame == (_last_written_frame + 1);
	}

	/* 3D */

	if (_last_written_eyes == EYES_LEFT && _queue.front().frame == _last_written_frame && _queue.front().eyes == EYES_RIGHT) {
		return true;
	}

	if (_last_written_eyes == EYES_RIGHT && _queue.front().frame == (_last_written_frame + 1) && _queue.front().eyes == EYES_LEFT) {
		return true;
	}

	return false;
}

void
Writer::thread ()
try
{
	while (true)
	{
		boost::mutex::scoped_lock lock (_mutex);

		/* This is for debugging only */
		bool done_something = false;

		while (true) {

			if (_finish || _queued_full_in_memory > _maximum_frames_in_memory || have_sequenced_image_at_queue_head ()) {
				/* We've got something to do: go and do it */
				break;
			}

			/* Nothing to do: wait until something happens which may indicate that we do */
			LOG_TIMING (N_("writer sleeps with a queue of %1"), _queue.size());
			_empty_condition.wait (lock);
			LOG_TIMING (N_("writer wakes with a queue of %1"), _queue.size());
		}

		/* We stop here if we have been asked to finish, and if either the queue
		   is empty or we do not have a sequenced image at its head (if this is the
		   case we will never terminate as no new frames will be sent once
		   _finish is true).
		*/
		if (_finish && (!have_sequenced_image_at_queue_head() || _queue.empty())) {
			done_something = true;
			/* (Hopefully temporarily) log anything that was not written */
			if (!_queue.empty() && !have_sequenced_image_at_queue_head()) {
				LOG_WARNING (N_("Finishing writer with a left-over queue of %1:"), _queue.size());
				for (list<QueueItem>::const_iterator i = _queue.begin(); i != _queue.end(); ++i) {
					LOG_WARNING (N_("- type %1, size %2, frame %3, eyes %4"), i->type, i->size, i->frame, i->eyes);
				}
				LOG_WARNING (N_("Last written frame %1, last written eyes %2"), _last_written_frame, _last_written_eyes);
			}
			return;
		}

		/* Write any frames that we can write; i.e. those that are in sequence. */
		while (have_sequenced_image_at_queue_head ()) {
			done_something = true;
			QueueItem qi = _queue.front ();
			_queue.pop_front ();
			if (qi.type == QueueItem::FULL && qi.encoded) {
				--_queued_full_in_memory;
			}

			lock.unlock ();
			switch (qi.type) {
			case QueueItem::FULL:
			{
				LOG_DEBUG (N_("Writer FULL-writes %1 to MXF"), qi.frame);
				if (!qi.encoded) {
					qi.encoded.reset (new EncodedData (_film->j2c_path (qi.frame, qi.eyes, false)));
				}

				libdcp::FrameInfo fin = _picture_asset_writer->write (qi.encoded->data(), qi.encoded->size());
				qi.encoded->write_info (_film, qi.frame, qi.eyes, fin);
				_last_written[qi.eyes] = qi.encoded;
				++_full_written;
				break;
			}
			case QueueItem::FAKE:
				LOG_DEBUG (N_("Writer FAKE-writes %1 to MXF"), qi.frame);
				_picture_asset_writer->fake_write (qi.size);
				_last_written[qi.eyes].reset ();
				++_fake_written;
				break;
			case QueueItem::REPEAT:
			{
				LOG_DEBUG (N_("Writer REPEAT-writes %1 to MXF"), qi.frame);
				libdcp::FrameInfo fin = _picture_asset_writer->write (
					_last_written[qi.eyes]->data(),
					_last_written[qi.eyes]->size()
					);

				_last_written[qi.eyes]->write_info (_film, qi.frame, qi.eyes, fin);
				++_repeat_written;
				break;
			}
			}
			lock.lock ();

			_last_written_frame = qi.frame;
			_last_written_eyes = qi.eyes;

			if (_film->length()) {
				shared_ptr<Job> job = _job.lock ();
				DCPOMATIC_ASSERT (job);
				int total = _film->time_to_video_frames (_film->length ());
				if (_film->three_d ()) {
					/* _full_written and so on are incremented for each eye, so we need to double the total
					   frames to get the correct progress.
					*/
					total *= 2;
				}
				job->set_progress (float (_full_written + _fake_written + _repeat_written) / total);
			}
		}

		while (_queued_full_in_memory > _maximum_frames_in_memory) {
			done_something = true;
			/* Too many frames in memory which can't yet be written to the stream.
			   Write some FULL frames to disk.
			*/

			/* Find one from the back of the queue */
			_queue.sort ();
			list<QueueItem>::reverse_iterator i = _queue.rbegin ();
			while (i != _queue.rend() && (i->type != QueueItem::FULL || !i->encoded)) {
				++i;
			}

			DCPOMATIC_ASSERT (i != _queue.rend());

			/* We will definitely write this data to disk, so clear it before we release the lock */
			shared_ptr<const EncodedData> to_write = i->encoded;
			i->encoded.reset ();

			/* Take a copy of the frame number and eyes so that we can unlock while we write */
			QueueItem qi = *i;

			++_pushed_to_disk;
			lock.unlock ();

			LOG_DEBUG (
				"Writer full with %1 (awaiting %2 [last eye was %3]); pushes %4 to disk",
				_queued_full_in_memory,
				_last_written_frame + 1,
				_last_written_eyes, qi.frame
				);

			to_write->write (_film, qi.frame, qi.eyes);

			lock.lock ();
			--_queued_full_in_memory;
		}

		if (!done_something) {
			LOG_DEBUG_NC ("Writer loop ran without doing anything");
			LOG_DEBUG ("_queued_full_in_memory=%1", _queued_full_in_memory);
			LOG_DEBUG ("_queue_size=%1", _queue.size ());
			LOG_DEBUG ("_finish=%1", _finish);
			LOG_DEBUG ("_last_written_frame=%1", _last_written_frame);
		}

		/* The queue has probably just gone down a bit; notify anything wait()ing on _full_condition */
		_full_condition.notify_all ();
	}
}
catch (...)
{
	store_current ();
}

void
Writer::terminate_thread (bool can_throw)
{
	boost::mutex::scoped_lock lock (_mutex);
	if (_thread == 0) {
		return;
	}

	_finish = true;
	_empty_condition.notify_all ();
	_full_condition.notify_all ();
	lock.unlock ();

 	_thread->join ();
	if (can_throw) {
		rethrow ();
	}

	delete _thread;
	_thread = 0;
}

void
Writer::finish ()
{
	if (!_thread) {
		return;
	}

	LOG_DEBUG_NC (N_("Terminating writer thread"));

	terminate_thread (true);

	LOG_DEBUG_NC (N_("Finalizing writers"));

	_picture_asset_writer->finalize ();
	if (_sound_asset_writer) {
		_sound_asset_writer->finalize ();
	}

	int const frames = _last_written_frame + 1;

	_picture_asset->set_duration (frames);

	/* Hard-link the video MXF into the DCP */
	LOG_GENERAL_NC (N_("Hard-linking video MXF into DCP"));

	boost::filesystem::path video_from = _picture_asset->path ();

	boost::filesystem::path video_to;
	video_to /= _film->dir (_film->dcp_name());
	video_to /= video_mxf_filename (_picture_asset);

	boost::system::error_code ec;
	boost::filesystem::create_hard_link (video_from, video_to, ec);
	if (ec) {
		LOG_WARNING ("Hard-link failed; copying instead (%1)", ec.message ());
		boost::filesystem::copy_file (video_from, video_to, ec);
		if (ec) {
			LOG_ERROR ("Failed to copy video file from %1 to %2 (%3)", video_from.string(), video_to.string(), ec.message ());
			throw FileError (ec.message(), video_from);
		}
	}

	/* And update the asset */

	_picture_asset->set_directory (_film->dir (_film->dcp_name ()));
	_picture_asset->set_file_name (video_mxf_filename (_picture_asset));

	/* Move the audio MXF into the DCP */
	LOG_GENERAL_NC (N_("Moving audio MXF into DCP"));

	if (_sound_asset) {
		boost::filesystem::path audio_to;
		audio_to /= _film->dir (_film->dcp_name ());
		audio_to /= audio_mxf_filename (_sound_asset);

		boost::filesystem::rename (_film->file (audio_mxf_filename (_sound_asset)), audio_to, ec);
		if (ec) {
			throw FileError (
				String::compose (_("could not move audio MXF into the DCP (%1)"), ec.value ()),
				_film->file (audio_mxf_filename (_sound_asset))
				);
		}

		_sound_asset->set_directory (_film->dir (_film->dcp_name ()));
		_sound_asset->set_duration (frames);
	}

	libdcp::DCP dcp (_film->dir (_film->dcp_name()));

	shared_ptr<libdcp::CPL> cpl (
		new libdcp::CPL (
			_film->dir (_film->dcp_name()),
			_film->dcp_name(),
			_film->dcp_content_type()->libdcp_kind (),
			frames,
			_film->video_frame_rate ()
			)
		);

	dcp.add_cpl (cpl);

	cpl->add_reel (shared_ptr<libdcp::Reel> (new libdcp::Reel (
							 _picture_asset,
							 _sound_asset,
							 shared_ptr<libdcp::SubtitleAsset> ()
							 )
			       ));

	shared_ptr<Job> job = _job.lock ();
	DCPOMATIC_ASSERT (job);

	job->sub (_("Computing image digest"));
	_picture_asset->compute_digest (boost::bind (&Job::set_progress, job.get(), _1, false));

	if (_sound_asset) {
		job->sub (_("Computing audio digest"));
		_sound_asset->compute_digest (boost::bind (&Job::set_progress, job.get(), _1, false));
	}

	libdcp::XMLMetadata meta;
	meta.issuer = Config::instance()->dcp_issuer ();
	meta.creator = String::compose ("DCP-o-matic %1 %2", dcpomatic_version, dcpomatic_git_commit);
	meta.set_issue_date_now ();
	dcp.write_xml (_film->interop (), meta, _film->is_signed() ? make_signer () : shared_ptr<const libdcp::Signer> ());

	LOG_GENERAL (
		N_("Wrote %1 FULL, %2 FAKE, %3 REPEAT; %4 pushed to disk"), _full_written, _fake_written, _repeat_written, _pushed_to_disk
		);
}

/** Tell the writer that frame `f' should be a repeat of the frame before it */
void
Writer::repeat (int f, Eyes e)
{
	boost::mutex::scoped_lock lock (_mutex);

	while (_queued_full_in_memory > _maximum_frames_in_memory) {
		/* The queue is too big; wait until that is sorted out */
		_full_condition.wait (lock);
	}

	QueueItem qi;
	qi.type = QueueItem::REPEAT;
	qi.frame = f;
	if (_film->three_d() && e == EYES_BOTH) {
		qi.eyes = EYES_LEFT;
		_queue.push_back (qi);
		qi.eyes = EYES_RIGHT;
		_queue.push_back (qi);
	} else {
		qi.eyes = e;
		_queue.push_back (qi);
	}

	/* Now there's something to do: wake anything wait()ing on _empty_condition */
	_empty_condition.notify_all ();
}

bool
Writer::check_existing_picture_mxf_frame (FILE* mxf, int f, Eyes eyes)
{
	/* Read the frame info as written */
	FILE* file = fopen_boost (_film->info_file (), "rb");
	if (!file) {
		LOG_GENERAL ("Existing frame %1 has no info file", f);
		return false;
	}
	libdcp::FrameInfo info = read_frame_info (file, f, eyes);
	fclose (file);

	/* Read the data from the MXF and hash it */
	dcpomatic_fseek (mxf, info.offset, SEEK_SET);
	EncodedData data (info.size);
	size_t const read = fread (data.data(), 1, data.size(), mxf);
	if (read != static_cast<size_t> (data.size ())) {
		LOG_GENERAL ("Existing frame %1 is incomplete", f);
		return false;
	}

	MD5Digester digester;
	digester.add (data.data(), data.size());
	if (digester.get() != info.hash) {
		LOG_GENERAL ("Existing frame %1 failed hash check", f);
		return false;
	}

	return true;
}

void
Writer::check_existing_picture_mxf ()
{
	/* Try to open the existing MXF */
	FILE* mxf = fopen_boost (_picture_asset->path(), "rb");
	if (!mxf) {
		LOG_GENERAL ("Could not open existing MXF at %1 (errno=%2)", _picture_asset->path().string(), errno);
		return;
	}

	while (true) {

		shared_ptr<Job> job = _job.lock ();
		DCPOMATIC_ASSERT (job);

		job->set_progress_unknown ();

		if (_film->three_d ()) {
			if (!check_existing_picture_mxf_frame (mxf, _first_nonexistant_frame, EYES_LEFT)) {
				break;
			}
			if (!check_existing_picture_mxf_frame (mxf, _first_nonexistant_frame, EYES_RIGHT)) {
				break;
			}
		} else {
			if (!check_existing_picture_mxf_frame (mxf, _first_nonexistant_frame, EYES_BOTH)) {
				break;
			}
		}

		LOG_GENERAL ("Have existing frame %1", _first_nonexistant_frame);
		++_first_nonexistant_frame;
	}

	fclose (mxf);
}

/** @param frame Frame index.
 *  @return true if we can fake-write this frame.
 */
bool
Writer::can_fake_write (int frame) const
{
	/* We have to do a proper write of the first frame so that we can set up the JPEG2000
	   parameters in the MXF writer.
	*/
	return (frame != 0 && frame < _first_nonexistant_frame);
}

bool
operator< (QueueItem const & a, QueueItem const & b)
{
	if (a.frame != b.frame) {
		return a.frame < b.frame;
	}

	return static_cast<int> (a.eyes) < static_cast<int> (b.eyes);
}

bool
operator== (QueueItem const & a, QueueItem const & b)
{
	return a.frame == b.frame && a.eyes == b.eyes;
}

void
Writer::set_encoder_threads (int threads)
{
	_maximum_frames_in_memory = rint (threads * 1.1);
}
