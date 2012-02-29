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
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include "player.h"
#include "film_state.h"
#include "filter.h"
#include "screen.h"
#include "exceptions.h"

using namespace std;
using namespace boost;

Player::Player (shared_ptr<const FilmState> fs, Screen const * screen, Split split)
{
	if (pipe (_mplayer_stdin) < 0) {
		throw PlayError ("could not create pipe");
	}

	if (pipe (_mplayer_stdout) < 0) {
		throw PlayError ("could not create pipe");
	}

	if (pipe (_mplayer_stderr) < 0) {
		throw PlayError ("could not create pipe");
	}
	
	int const p = fork ();
	if (p < 0) {
		cerr << "**** could not fork for mplayer\n";
		throw PlayError ("could not fork for mplayer");
	} else if (p == 0) {
		close (_mplayer_stdin[1]);
		dup2 (_mplayer_stdin[0], STDIN_FILENO);
		
		close (_mplayer_stdout[0]);
		dup2 (_mplayer_stdout[1], STDOUT_FILENO);
		
		close (_mplayer_stderr[0]);
		dup2 (_mplayer_stderr[1], STDERR_FILENO);

		char* p[] = { strdup ("TERM=xterm"), strdup ("DISPLAY=:0"), strdup ("HOME=/home/carl"), strdup ("LOGNAME=/home/carl/fucker.log"), 0 };
		environ = p;

		stringstream s;
		s << "/usr/local/bin/mplayer";

		s << " -vo x11 -noaspect -nosound -noautosub -nosub -vo x11 -noborder -slave -quiet"; // -idle
		s << " -sws " << fs->scaler->mplayer_id ();

		stringstream vf;
		
		Position position = screen->position (fs->format);
		Size screen_size = screen->size (fs->format);
		Size const cropped_size = fs->cropped_size (fs->size);
		switch (split) {
		case SPLIT_NONE:
			vf << crop_string (Position (fs->left_crop, fs->top_crop), cropped_size);
			s << " -geometry " << position.x << ":" << position.y;
			break;
		case SPLIT_LEFT:
		{
			Size split_size = cropped_size;
			split_size.width /= 2;
			vf << crop_string (Position (fs->left_crop, fs->top_crop), split_size);
			screen_size.width /= 2;
			s << " -geometry " << position.x << ":" << position.y;
			break;
		}
		case SPLIT_RIGHT:
		{
			Size split_size = cropped_size;
			split_size.width /= 2;
			vf << crop_string (Position (fs->left_crop + split_size.width, fs->top_crop), split_size);
			screen_size.width /= 2;
			s << " -geometry " << (position.x + screen_size.width) << ":" << position.y;
			break;
		}
		}

		vf << ",scale=" << screen_size.width << ":" << screen_size.height;
		
		pair<string, string> filters = Filter::ffmpeg_strings (fs->filters);
		
		if (!filters.first.empty()) {
			vf << "," << filters.first;
		}
		
		if (!filters.second.empty ()) {
			vf << ",pp=" << filters.second;
		}
		
		s << " -vf " << vf.str();
		s << " " << fs->file (fs->content);

		string cmd (s.str ());
		cout << cmd << "\n";

		vector<string> b;
		boost::split (b, cmd, is_any_of (" "));
		
		char** cl = new char*[b.size() + 1];
		for (vector<string>::size_type i = 0; i < b.size(); ++i) {
			cl[i] = strdup (b[i].c_str ());
		}
		cl[b.size()] = 0;
		
		execv (cl[0], cl);

		stringstream e;
		cerr << "**** exec of mplayer failed " << strerror (errno) << "\n";
		e << "exec of mplayer failed " << strerror (errno);
		throw PlayError (e.str ());
		
	} else {
		_mplayer_pid = p;
		command ("pause");
	}
}

Player::~Player ()
{
	close (_mplayer_stdin[0]);
	close (_mplayer_stdout[1]);
	kill (_mplayer_pid, SIGTERM);
}

void
Player::command (string c)
{
	char buf[64];
	snprintf (buf, sizeof (buf), "%s\n", c.c_str ());
	write (_mplayer_stdin[1], buf, strlen (buf));
}

string
Player::command_with_reply (string c, string t)
{
	command (c);

	struct pollfd pfd[1];
	pfd[0].fd = _mplayer_stdout[0];
	pfd[0].events = POLLIN;
	poll (pfd, 1, 50);
	
	if ((pfd[0].revents & POLLIN) == 0) {
		return "";
	}

	/* XXX: this may not be enough; should probably be looping */
	char buf[2048];
	int r = read (_mplayer_stdout[0], buf, sizeof (buf));
	if (r < 0) {
		return "";
	}

	stringstream s (buf);
	while (s.good ()) {
		string line;
		getline (s, line);

		vector<string> b;
		string s (buf);
		trim (line);
		split (b, line, is_any_of ("="));
		if (b.size() < 2) {
			return "";
		}
		
		if (b[0] == t) {
			return b[1];
		}
	}
		
	return "";
}
