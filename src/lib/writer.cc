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

#include <fstream>
#include <cerrno>
#include <dcp/mono_picture_mxf.h>
#include <dcp/stereo_picture_mxf.h>
#include <dcp/sound_mxf.h>
#include <dcp/sound_mxf_writer.h>
#include <dcp/reel.h>
#include <dcp/reel_mono_picture_asset.h>
#include <dcp/reel_stereo_picture_asset.h>
#include <dcp/reel_sound_asset.h>
#include <dcp/reel_subtitle_asset.h>
#include <dcp/dcp.h>
#include <dcp/cpl.h>
#include <dcp/signer.h>
#include "writer.h"
#include "compose.hpp"
#include "film.h"
#include "ratio.h"
#include "log.h"
#include "dcp_video.h"
#include "dcp_content_type.h"
#include "audio_mapping.h"
#include "config.h"
#include "job.h"
#include "cross.h"
#include "audio_buffers.h"
#include "md5_digester.h"
#include "encoded_data.h"

#include "i18n.h"

#define LOG_GENERAL(...) _film->log()->log (String::compose (__VA_ARGS__), Log::TYPE_GENERAL);
#define LOG_TIMING(...) _film->log()->microsecond_log (String::compose (__VA_ARGS__), Log::TYPE_TIMING);
#define LOG_WARNING_NC(...) _film->log()->log (__VA_ARGS__, Log::TYPE_WARNING);

/* OS X strikes again */
#undef set_key

using std::make_pair;
using std::pair;
using std::string;
using std::list;
using std::cout;
using std::stringstream;
using boost::shared_ptr;
using boost::weak_ptr;
using boost::dynamic_pointer_cast;

int const Writer::_maximum_frames_in_memory = Config::instance()->num_local_encoding_threads() + 4;

Writer::Writer (shared_ptr<const Film> f, weak_ptr<Job> j)
	: _film (f)
	, _job (j)
	, _first_nonexistant_frame (0)
	, _thread (0)
	, _finish (false)
	, _queued_full_in_memory (0)
	, _last_written_frame (-1)
	, _last_written_eyes (EYES_RIGHT)
	, _full_written (0)
	, _fake_written (0)
	, _pushed_to_disk (0)
{
	/* Remove any old DCP */
	boost::filesystem::remove_all (_film->dir (_film->dcp_name ()));

	shared_ptr<Job> job = _job.lock ();
	assert (job);

	job->sub (_("Checking existing image data"));
	check_existing_picture_mxf ();

	/* Create our picture asset in a subdirectory, named according to those
	   film's parameters which affect the video output.  We will hard-link
	   it into the DCP later.
	*/

	if (_film->three_d ()) {
		_picture_mxf.reset (new dcp::StereoPictureMXF (dcp::Fraction (_film->video_frame_rate (), 1)));
	} else {
		_picture_mxf.reset (new dcp::MonoPictureMXF (dcp::Fraction (_film->video_frame_rate (), 1)));
	}

	_picture_mxf->set_size (_film->frame_size ());

	if (_film->encrypted ()) {
		_picture_mxf->set_key (_film->key ());
	}
	
	_picture_mxf_writer = _picture_mxf->start_write (
		_film->internal_video_mxf_dir() / _film->internal_video_mxf_filename(),
		_film->interop() ? dcp::INTEROP : dcp::SMPTE,
		_first_nonexistant_frame > 0
		);

	if (_film->audio_channels ()) {
		_sound_mxf.reset (new dcp::SoundMXF (dcp::Fraction (_film->video_frame_rate(), 1), _film->audio_frame_rate (), _film->audio_channels ()));

		if (_film->encrypted ()) {
			_sound_mxf->set_key (_film->key ());
		}
	
		/* Write the sound MXF into the film directory so that we leave the creation
		   of the DCP directory until the last minute.
		*/
		_sound_mxf_writer = _sound_mxf->start_write (_film->directory() / _film->audio_mxf_filename(), _film->interop() ? dcp::INTEROP : dcp::SMPTE);
	}

	/* Check that the signer is OK if we need one */
	if (_film->is_signed() && !Config::instance()->signer()->valid ()) {
		throw InvalidSignerError ();
	}

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
	
	FILE* ifi = fopen_boost (_film->info_path (frame, eyes), "r");
	dcp::FrameInfo info (ifi);
	fclose (ifi);
	
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
	if (_sound_mxf_writer) {
		_sound_mxf_writer->write (audio->data(), audio->frames());
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

		if (_finish && _queue.empty()) {
			return;
		}

		/* Write any frames that we can write; i.e. those that are in sequence. */
		while (have_sequenced_image_at_queue_head ()) {
			QueueItem qi = _queue.front ();
			_queue.pop_front ();
			if (qi.type == QueueItem::FULL && qi.encoded) {
				--_queued_full_in_memory;
			}

			lock.unlock ();
			switch (qi.type) {
			case QueueItem::FULL:
			{
				LOG_GENERAL (N_("Writer FULL-writes %1 to MXF"), qi.frame);
				if (!qi.encoded) {
					qi.encoded.reset (new EncodedData (_film->j2c_path (qi.frame, qi.eyes, false)));
				}

				dcp::FrameInfo fin = _picture_mxf_writer->write (qi.encoded->data(), qi.encoded->size());
				qi.encoded->write_info (_film, qi.frame, qi.eyes, fin);
				_last_written[qi.eyes] = qi.encoded;
				++_full_written;
				break;
			}
			case QueueItem::FAKE:
				LOG_GENERAL (N_("Writer FAKE-writes %1 to MXF"), qi.frame);
				_picture_mxf_writer->fake_write (qi.size);
				_last_written[qi.eyes].reset ();
				++_fake_written;
				break;
			}
			lock.lock ();

			_last_written_frame = qi.frame;
			_last_written_eyes = qi.eyes;
			
			shared_ptr<Job> job = _job.lock ();
			assert (job);
			int64_t total = _film->length().frames (_film->video_frame_rate ());
			if (_film->three_d ()) {
				/* _full_written and so on are incremented for each eye, so we need to double the total
				   frames to get the correct progress.
				*/
				total *= 2;
			}
			if (total) {
				job->set_progress (float (_full_written + _fake_written) / total);
			}
		}

		while (_queued_full_in_memory > _maximum_frames_in_memory) {
			/* Too many frames in memory which can't yet be written to the stream.
			   Write some FULL frames to disk.
			*/

			/* Find one from the back of the queue */
			_queue.sort ();
			list<QueueItem>::reverse_iterator i = _queue.rbegin ();
			while (i != _queue.rend() && (i->type != QueueItem::FULL || !i->encoded)) {
				++i;
			}

			assert (i != _queue.rend());
			QueueItem qi = *i;

			++_pushed_to_disk;
			
			lock.unlock ();

			LOG_GENERAL (
				"Writer full (awaiting %1 [last eye was %2]); pushes %3 to disk",
				_last_written_frame + 1,
				_last_written_eyes, qi.frame
				);
			
			qi.encoded->write (_film, qi.frame, qi.eyes);
			lock.lock ();
			qi.encoded.reset ();
			--_queued_full_in_memory;
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
	
	terminate_thread (true);

	_picture_mxf_writer->finalize ();
	if (_sound_mxf_writer) {
		_sound_mxf_writer->finalize ();
	}
	
	/* Hard-link the video MXF into the DCP */
	boost::filesystem::path video_from;
	video_from /= _film->internal_video_mxf_dir();
	video_from /= _film->internal_video_mxf_filename();
	
	boost::filesystem::path video_to;
	video_to /= _film->dir (_film->dcp_name());
	video_to /= _film->video_mxf_filename ();

	boost::system::error_code ec;
	boost::filesystem::create_hard_link (video_from, video_to, ec);
	if (ec) {
		/* hard link failed; copy instead */
		boost::filesystem::copy_file (video_from, video_to);
		LOG_WARNING_NC ("Hard-link failed; fell back to copying");
	}

	_picture_mxf->set_file (video_to);

	/* Move the audio MXF into the DCP */

	if (_sound_mxf) {
		boost::filesystem::path audio_to;
		audio_to /= _film->dir (_film->dcp_name ());
		audio_to /= _film->audio_mxf_filename ();
		
		boost::filesystem::rename (_film->file (_film->audio_mxf_filename ()), audio_to, ec);
		if (ec) {
			throw FileError (
				String::compose (_("could not move audio MXF into the DCP (%1)"), ec.value ()), _film->file (_film->audio_mxf_filename ())
				);
		}

		_sound_mxf->set_file (audio_to);
	}

	dcp::DCP dcp (_film->dir (_film->dcp_name()));

	shared_ptr<dcp::CPL> cpl (
		new dcp::CPL (
			_film->dcp_name(),
			_film->dcp_content_type()->libdcp_kind ()
			)
		);
	
	dcp.add (cpl);

	shared_ptr<dcp::Reel> reel (new dcp::Reel ());

	shared_ptr<dcp::MonoPictureMXF> mono = dynamic_pointer_cast<dcp::MonoPictureMXF> (_picture_mxf);
	if (mono) {
		reel->add (shared_ptr<dcp::ReelPictureAsset> (new dcp::ReelMonoPictureAsset (mono, 0)));
		dcp.add (mono);
	}

	shared_ptr<dcp::StereoPictureMXF> stereo = dynamic_pointer_cast<dcp::StereoPictureMXF> (_picture_mxf);
	if (stereo) {
		reel->add (shared_ptr<dcp::ReelPictureAsset> (new dcp::ReelStereoPictureAsset (stereo, 0)));
		dcp.add (stereo);
	}

	if (_sound_mxf) {
		reel->add (shared_ptr<dcp::ReelSoundAsset> (new dcp::ReelSoundAsset (_sound_mxf, 0)));
		dcp.add (_sound_mxf);
	}

	if (_subtitle_content) {
		_subtitle_content->write_xml (_film->dir (_film->dcp_name ()) / _film->subtitle_xml_filename ());
		reel->add (shared_ptr<dcp::ReelSubtitleAsset> (
				   new dcp::ReelSubtitleAsset (
					   _subtitle_content,
					   dcp::Fraction (_film->video_frame_rate(), 1),
					   _picture_mxf->intrinsic_duration (),
					   0
					   )
				   ));
		
		dcp.add (_subtitle_content);
	}
	
	cpl->add (reel);

	shared_ptr<Job> job = _job.lock ();
	assert (job);

	job->sub (_("Computing image digest"));
	_picture_mxf->hash (boost::bind (&Job::set_progress, job.get(), _1, false));

	if (_sound_mxf) {
		job->sub (_("Computing audio digest"));
		_sound_mxf->hash (boost::bind (&Job::set_progress, job.get(), _1, false));
	}

	dcp::XMLMetadata meta = Config::instance()->dcp_metadata ();
	meta.set_issue_date_now ();

	shared_ptr<const dcp::Signer> signer;
	if (_film->is_signed ()) {
		signer = Config::instance()->signer ();
		/* We did check earlier, but check again here to be on the safe side */
		if (!signer->valid ()) {
			throw InvalidSignerError ();
		}
	}

	dcp.write_xml (_film->interop () ? dcp::INTEROP : dcp::SMPTE, meta, signer);

	LOG_GENERAL (
		N_("Wrote %1 FULL, %2 FAKE, %3 pushed to disk"), _full_written, _fake_written, _pushed_to_disk
		);
}

bool
Writer::check_existing_picture_mxf_frame (FILE* mxf, int f, Eyes eyes)
{
	/* Read the frame info as written */
	FILE* ifi = fopen_boost (_film->info_path (f, eyes), "r");
	if (!ifi) {
		LOG_GENERAL ("Existing frame %1 has no info file", f);
		return false;
	}
	
	dcp::FrameInfo info (ifi);
	fclose (ifi);
	if (info.size == 0) {
		LOG_GENERAL ("Existing frame %1 has no info file", f);
		return false;
	}
	
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
	boost::filesystem::path p;
	p /= _film->internal_video_mxf_dir ();
	p /= _film->internal_video_mxf_filename ();
	FILE* mxf = fopen_boost (p, "rb");
	if (!mxf) {
		LOG_GENERAL ("Could not open existing MXF at %1 (errno=%2)", p.string(), errno);
		return;
	}

	int N = 0;
	for (boost::filesystem::directory_iterator i (_film->info_dir ()); i != boost::filesystem::directory_iterator (); ++i) {
		++N;
	}

	while (true) {

		shared_ptr<Job> job = _job.lock ();
		assert (job);

		if (N > 0) {
			job->set_progress (float (_first_nonexistant_frame) / N);
		}

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

void
Writer::write (PlayerSubtitles subs)
{
	if (subs.text.empty ()) {
		return;
	}
	
	if (!_subtitle_content) {
		_subtitle_content.reset (
			new dcp::SubtitleContent (_film->name(), _film->isdcf_metadata().subtitle_language)
			);
	}
	
	for (list<dcp::SubtitleString>::const_iterator i = subs.text.begin(); i != subs.text.end(); ++i) {
		_subtitle_content->add (*i);
	}
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
