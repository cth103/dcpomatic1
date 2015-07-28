/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#include <boost/signals2.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread.hpp>
#include <curl/curl.h>

struct update_checker_test;

class UpdateChecker
{
public:
	UpdateChecker ();
	~UpdateChecker ();

	void run ();

	enum State {
		YES,
		FAILED,
		NO,
		NOT_RUN
	};

	State state () {
		boost::mutex::scoped_lock lm (_data_mutex);
		return _state;
	}

	/** @return new stable version, if there is one */
	boost::optional<std::string> stable () {
		boost::mutex::scoped_lock lm (_data_mutex);
		return _stable;
	}

	/** @return new test version, if there is one and Config is set to look for it */
	boost::optional<std::string> test () {
		boost::mutex::scoped_lock lm (_data_mutex);
		return _test;
	}

	/** @return true if the list signal emission was the first */
	bool last_emit_was_first () const {
		boost::mutex::scoped_lock lm (_data_mutex);
		return _emits == 1;
	}

	size_t write_callback (void *, size_t, size_t);

	boost::signals2::signal<void (void)> StateChanged;

	static UpdateChecker* instance ();

private:
	friend struct update_checker_test;

	static UpdateChecker* _instance;

	static bool version_less_than (std::string const & a, std::string const & b);

	void set_state (State);
	void thread ();

	char* _buffer;
	int _offset;
	CURL* _curl;

	/** mutex to protect _state, _stable, _test and _emits */
	mutable boost::mutex _data_mutex;
	State _state;
	boost::optional<std::string> _stable;
	boost::optional<std::string> _test;
	int _emits;

	boost::thread* _thread;
	boost::mutex _process_mutex;
	boost::condition _condition;
	int _to_do;
};
