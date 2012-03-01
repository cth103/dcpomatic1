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

#include <gtkmm.h>

class Film;
class Screen;

class FilmPlayer
{
public:
	FilmPlayer (Film const *);
	
	Gtk::Widget& widget ();
	
	void set_film (Film const *);

private:
	void play_clicked ();
	void pause_clicked ();
	void stop_clicked ();
	
	void set_button_states ();
	Screen * screen () const;
	
	Film const * _film;

	Gtk::Table _table;
	Gtk::Button _play;
	Gtk::Button _pause;
	Gtk::Button _stop;
	Gtk::ComboBoxText _screen;
};
