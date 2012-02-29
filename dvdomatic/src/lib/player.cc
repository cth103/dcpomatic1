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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/thread.hpp>
#include "player.h"
#include "film_state.h"
#include "filter.h"
#include "screen.h"
#include "exceptions.h"

using namespace std;
using namespace boost;

Player::Player (shared_ptr<const FilmState> fs, Screen const * sc, Split s, string f)
	: _fs (fs)
	, _screen (sc)
	, _split (s)
	, _mplayer (0)
	, _fifo_file (f)
	, _fifo (0)
	, _state (NOT_STARTED)
{
	
}

Player::~Player ()
{
	if (_mplayer) {
		pclose (_mplayer);
	}

	if (_fifo) {
		close (_fifo);
	}

	unlink (_fifo_file.c_str ());
}

void
Player::run_mplayer ()
{
	mkfifo (_fifo_file.c_str(), 0600);

	stringstream s;
	s << "mplayer ";

	s << "-vo x11 -noaspect -ao pulse -noautosub -nosub -vo x11 -noborder -slave -input file=" << _fifo_file << " ";
	s << "-sws " << _fs->scaler->mplayer_id () << " ";

	stringstream vf;

	Position position = _screen->position (_fs->format);
	Size size = _screen->size (_fs->format);
	Size const cropped = _fs->cropped_size (_fs->size);
	switch (_split) {
	case SPLIT_NONE:
		vf << crop_string (Position (_fs->left_crop, _fs->top_crop), Size (_fs->cropped_size (_fs->size)));
		s << "-geometry " << position.x << ":" << position.y;
		break;
	case SPLIT_LEFT:
	{
		Size split = cropped;
		split.width /= 2;
		vf << crop_string (Position (_fs->left_crop, _fs->top_crop), split);
		size.width /= 2;
		s << "-geometry " << position.x << ":" << position.y;
		break;
	}
	case SPLIT_RIGHT:
	{
		Size split = cropped;
		split.width /= 2;
		vf << crop_string (Position (_fs->left_crop + split.width, _fs->top_crop), split);
		size.width /= 2;
		s << "-geometry " << (position.x + size.width) << ":" << position.y;
		break;
	}
	}

	vf << ",scale=" << size.width << ":" << size.height;

	pair<string, string> filters = Filter::ffmpeg_strings (_fs->filters);

	if (!filters.first.empty()) {
		vf << "," << filters.first;
	}

	if (!filters.second.empty ()) {
		vf << ",pp=" << filters.second;
	}

	s << " -vf " << vf.str();
	s << " \"" << _fs->file (_fs->content) << "\"";

	cout << s.str() << "\n";

	_mplayer = popen (s.str().c_str (), "r");
	if (_mplayer == 0) {
		stringstream s;
		s << "could not popen() mplayer " << strerror (errno);
		throw PlayError (s.str ());
	}

	_fifo = open (_fifo_file.c_str(), O_WRONLY);
	if (_fifo == -1) {
		throw PlayError ("could not open FIFO to mplayer");
	}
}

void
Player::stop ()
{
	switch (_state) {
	case PLAYING:
		command ("pause");
		_state = PAUSED;
		break;
	}
}

void
Player::play ()
{
	switch (_state) {
	case NOT_STARTED:
		run_mplayer ();
		_state = PLAYING;
		break;
	case PAUSED:
		command ("pause");
		_state = PLAYING;
		break;
	default:
		break;
	}
}

void
Player::command (string c)
{
	char buf[64];
	snprintf (buf, sizeof (buf), "%s\n", c.c_str ());
	write (_fifo, buf, strlen (buf));
}
