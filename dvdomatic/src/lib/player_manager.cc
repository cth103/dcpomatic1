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

#include "player_manager.h"
#include "player.h"
#include "film_state.h"
#include "screen.h"

using namespace std;
using namespace boost;

PlayerManager* PlayerManager::_instance = 0;

PlayerManager::PlayerManager ()
{

}

PlayerManager *
PlayerManager::instance ()
{
	if (_instance == 0) {
		_instance = new PlayerManager ();
	}

	return _instance;
}

void
PlayerManager::setup (shared_ptr<const FilmState> fs, Screen const * sc)
{
	_players.clear ();
	_players.push_back (shared_ptr<Player> (new Player (fs, sc, Player::SPLIT_NONE)));
}

void
PlayerManager::setup (shared_ptr<const FilmState> fs_a, shared_ptr<const FilmState> fs_b, Screen const * sc)
{
	_players.clear ();

	_players.push_back (shared_ptr<Player> (new Player (fs_a, sc, Player::SPLIT_LEFT)));
	_players.push_back (shared_ptr<Player> (new Player (fs_b, sc, Player::SPLIT_RIGHT)));
}

void
PlayerManager::play ()
{
	for (list<shared_ptr<Player> >::iterator i = _players.begin(); i != _players.end(); ++i) {
		(*i)->command ("pause");
	}
}

void
PlayerManager::stop ()
{
	for (list<shared_ptr<Player> >::iterator i = _players.begin(); i != _players.end(); ++i) {
		(*i)->command ("pause");
	}
}
