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

#include "lib/screen.h"
#include "lib/config.h"
#include "lib/player_manager.h"
#include "lib/film.h"
#include "film_player.h"
#include "gtk_util.h"

using namespace std;
using namespace boost;

FilmPlayer::FilmPlayer (Film const * f)
	: _play ("Play")
	, _pause ("Pause")
	, _stop ("Stop")
	, _ab ("A/B")
{
	set_film (f);

	vector<Screen *> const scr = Config::instance()->screens ();
	for (vector<Screen *>::const_iterator i = scr.begin(); i != scr.end(); ++i) {
		_screen.append_text ((*i)->name ());
	}
	
	if (!scr.empty ()) {
		_screen.set_active (0);
	}

	_status.set_use_markup (true);

	_table.set_row_spacings (4);
	_table.set_col_spacings (12);

	_table.attach (_play, 0, 1, 0, 1);
	_table.attach (_pause, 0, 1, 1, 2);
	_table.attach (_stop, 0, 1, 2, 3);
	_table.attach (_screen, 1, 2, 0, 1);
	_table.attach (_ab, 1, 2, 1, 2);
	_table.attach (_status, 0, 2, 3, 4);

	_play.signal_clicked().connect (sigc::mem_fun (*this, &FilmPlayer::play_clicked));
	_pause.signal_clicked().connect (sigc::mem_fun (*this, &FilmPlayer::pause_clicked));
	_stop.signal_clicked().connect (sigc::mem_fun (*this, &FilmPlayer::stop_clicked));

	set_button_states ();
	Glib::signal_timeout().connect (sigc::bind_return (sigc::mem_fun (*this, &FilmPlayer::update), true), 1000);
}

void
FilmPlayer::set_film (Film const * f)
{
	_film = f;
}

Gtk::Widget &
FilmPlayer::widget ()
{
	return _table;
}

void
FilmPlayer::set_button_states ()
{
	PlayerManager::State s = PlayerManager::instance()->state ();

	switch (s) {
	case PlayerManager::QUIESCENT:
		_play.set_sensitive (true);
		_pause.set_sensitive (false);
		_stop.set_sensitive (false);
		_screen.set_sensitive (true);
		break;
	case PlayerManager::PLAYING:
		_play.set_sensitive (false);
		_pause.set_sensitive (true);
		_stop.set_sensitive (true);
		_screen.set_sensitive (false);
		break;
	case PlayerManager::PAUSED:
		_play.set_sensitive (true);
		_pause.set_sensitive (false);
		_stop.set_sensitive (true);
		_screen.set_sensitive (false);
		break;
	}
}

void
FilmPlayer::play_clicked ()
{
	PlayerManager* p = PlayerManager::instance ();

	switch (p->state ()) {
	case PlayerManager::QUIESCENT:
		_last_play_fs = _film->state_copy ();
		if (_ab.get_active ()) {
			shared_ptr<FilmState> fs_a = _film->state_copy ();
			fs_a->filters.clear ();
			/* This is somewhat arbitrary, but hey ho */
			fs_a->scaler = Scaler::get_from_id ("bicubic");
			p->setup (fs_a, _last_play_fs, screen ());
		} else {
			p->setup (_last_play_fs, screen ());
		}
		p->pause_or_unpause ();
		break;
	case PlayerManager::PLAYING:
		break;
	case PlayerManager::PAUSED:
		p->pause_or_unpause ();
		break;
	}
}

void
FilmPlayer::pause_clicked ()
{
	PlayerManager* p = PlayerManager::instance ();

	switch (p->state ()) {
	case PlayerManager::QUIESCENT:
		break;
	case PlayerManager::PLAYING:
		p->pause_or_unpause ();
		break;
	case PlayerManager::PAUSED:
		break;
	}
}

void
FilmPlayer::stop_clicked ()
{
	PlayerManager::instance()->stop ();
}

Screen *
FilmPlayer::screen () const
{
	vector<Screen *> const s = Config::instance()->screens ();
	if (s.empty ()) {
		return 0;
	}
			 
	int const r = _screen.get_active_row_number ();
	if (r >= int (s.size ())) {
		return s[0];
	}
	
	return s[r];
}

void
FilmPlayer::update ()
{
	set_button_states ();
	set_status ();
}

void
FilmPlayer::set_status ()
{
	PlayerManager::State s = PlayerManager::instance()->state ();

	stringstream m;
	switch (s) {
	case PlayerManager::QUIESCENT:
		m << "Idle";
		break;
	case PlayerManager::PLAYING:
		m << "<span foreground=\"red\" weight=\"bold\">PLAYING</span>";
		break;
	case PlayerManager::PAUSED:
		m << "<b>Paused</b>";
		break;
	}

	if (s != PlayerManager::QUIESCENT) {
		float const p = PlayerManager::instance()->position ();
		m << " " << seconds_to_hms (p);
		if (_last_play_fs->frames_per_second != 0 && _last_play_fs->length != 0) {
			m << " <i>(" << seconds_to_hms (_last_play_fs->length / _last_play_fs->frames_per_second - p) << " remaining)</i>";
		}
	}

	_status.set_markup (m.str ());
}

#if 0
void
FilmEditor::setup_player_manager ()
{
	stop_updating_play_position ();

	vector<Screen *> screens = Config::instance()->screens ();
	if (screens.empty ()) {
		return;
	}

	Screen* screen = 0;
	if (_play_screen.get_active_row_number() >= int (screens.size ())) {
		_play_screen.set_active (0);
		screen = screens.front ();
	} else {
		screen = screens[_play_screen.get_active_row_number()];
	}
       
	if (_play_ab.get_active ()) {
		shared_ptr<FilmState> fs_a = _film->state_copy ();
		fs_a->filters.clear ();
		/* This is somewhat arbitrary, but hey ho */
		fs_a->scaler = Scaler::get_from_id ("bicubic");
		PlayerManager::instance()->setup (fs_a, _film->state_copy(), screen);
	} else {
		PlayerManager::instance()->setup (_film->state_copy(), screen);
	}
}
#endif

