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
PlayerManager::add (shared_ptr<Player> p)
{
	_players.push_back (p);
}
