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

#include <list>
#include <vector>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <sndfile.h>
#include "encoder.h"
#include "image.h"

class Job;

class J2KWAVEncoder : public Encoder
{
public:
	J2KWAVEncoder (Parameters const *);
	~J2KWAVEncoder ();

	void set_decoder (Decoder *);

	void process_begin ();
	void process_video (uint8_t *, int, int);
	void process_audio (uint8_t *, int, int);
	void process_end ();

private:	

	void encoder_thread ();

	std::string wav_path (int, bool) const;

	std::vector<SNDFILE*> _sound_files;
	int _deinterleave_buffer_size;
	uint8_t* _deinterleave_buffer;

	bool _process_end;
	std::list<boost::shared_ptr<Image> > _queue;
	std::list<boost::thread *> _worker_threads;
	int _num_worker_threads;
	boost::mutex _worker_mutex;
	boost::condition _worker_condition;
};
