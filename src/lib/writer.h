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

#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include "exceptions.h"
#include "types.h"

class Film;
class EncodedData;
class AudioBuffers;
class Job;

namespace libdcp {
	class MonoPictureAsset;
	class MonoPictureAssetWriter;
	class StereoPictureAsset;
	class StereoPictureAssetWriter;
	class PictureAsset;
	class PictureAssetWriter;
	class SoundAsset;
	class SoundAssetWriter;
}

struct QueueItem
{
public:
	enum Type {
		/** a normal frame with some JPEG200 data */
		FULL,
		/** a frame whose data already exists in the MXF,
		    and we fake-write it; i.e. we update the writer's
		    state but we use the data that is already on disk.
		*/
		FAKE,
		/** this is a repeat of the last frame to be written */
		REPEAT
	} type;

	/** encoded data for FULL */
	boost::shared_ptr<const EncodedData> encoded;
	/** size of data for FAKE */
	int size;
	/** frame index */
	int frame;
	Eyes eyes;
};

bool operator< (QueueItem const & a, QueueItem const & b);
bool operator== (QueueItem const & a, QueueItem const & b);

class Writer : public ExceptionStore, public boost::noncopyable
{
public:
	Writer (boost::shared_ptr<const Film>, boost::weak_ptr<Job>);
	~Writer ();

	bool can_fake_write (int) const;

	void write (boost::shared_ptr<const EncodedData>, int, Eyes);
	void fake_write (int, Eyes);
	void write (boost::shared_ptr<const AudioBuffers>);
	void repeat (int f, Eyes);
	void finish ();

	void set_encoder_threads (int threads);

private:

	void thread ();
	void terminate_thread (bool);
	void check_existing_picture_mxf ();
	bool check_existing_picture_mxf_frame (FILE *, int, Eyes);
	bool have_sequenced_image_at_queue_head ();

	/** our Film */
	boost::shared_ptr<const Film> _film;
	boost::weak_ptr<Job> _job;
	/** the first frame index that does not already exist in our MXF */
	int _first_nonexistant_frame;

	/** our thread, or 0 */
	boost::thread* _thread;
	/** true if our thread should finish */
	bool _finish;
	/** queue of things to write to disk */
	std::list<QueueItem> _queue;
	/** number of FULL frames whose JPEG200 data is currently held in RAM */
	int _queued_full_in_memory;
	/** mutex for thread state */
	mutable boost::mutex _mutex;
	/** condition to manage thread wakeups when we have nothing to do  */
	boost::condition _empty_condition;
	/** condition to manage thread wakeups when we have too much to do */
	boost::condition _full_condition;
	/** the data of the last written frame, or 0 if there isn't one */
	boost::shared_ptr<const EncodedData> _last_written[EYES_COUNT];
	/** the index of the last written frame */
	int _last_written_frame;
	Eyes _last_written_eyes;
	/** maximum number of frames to hold in memory, for when we are managing
	    ordering
	*/
	int _maximum_frames_in_memory;

	/** number of FULL written frames */
	int _full_written;
	/** number of FAKE written frames */
	int _fake_written;
	/** number of REPEAT written frames */
	int _repeat_written;
	/** number of frames pushed to disk and then recovered
	    due to the limit of frames to be held in memory.
	*/
	int _pushed_to_disk;

	boost::shared_ptr<libdcp::PictureAsset> _picture_asset;
	boost::shared_ptr<libdcp::PictureAssetWriter> _picture_asset_writer;
	boost::shared_ptr<libdcp::SoundAsset> _sound_asset;
	boost::shared_ptr<libdcp::SoundAssetWriter> _sound_asset_writer;
};
