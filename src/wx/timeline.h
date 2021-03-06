/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/signals2.hpp>
#include <wx/wx.h>
#include "lib/util.h"
#include "lib/rect.h"
#include "content_menu.h"

class Film;
class TimelineView;
class TimelineContentView;
class FilmEditor;
class TimelineTimeAxisView;

class Timeline : public wxPanel
{
public:
	Timeline (wxWindow *, FilmEditor *, boost::shared_ptr<Film>);

	boost::shared_ptr<const Film> film () const;

	void force_redraw (dcpomatic::Rect<int> const &);

	int x_offset () const {
		return 8;
	}

	int width () const {
		return GetSize().GetWidth ();
	}

	int track_height () const {
		return 48;
	}

	boost::optional<double> pixels_per_time_unit () const {
		return _pixels_per_time_unit;
	}

	Position<int> tracks_position () const {
		return Position<int> (8, 8);
	}

	int tracks () const;

	void setup_pixels_per_time_unit ();

	void set_snap (bool s) {
		_snap = s;
	}

	bool snap () const {
		return _snap;
	}

private:
	void paint ();
	void left_down (wxMouseEvent &);
	void left_up (wxMouseEvent &);
	void right_down (wxMouseEvent &);
	void mouse_moved (wxMouseEvent &);
	void playlist_changed ();
	void playlist_content_changed (int);
	void resized ();
	void assign_tracks ();
	void set_position_from_event (wxMouseEvent &);
	void clear_selection ();
	void recreate_views ();

	boost::shared_ptr<TimelineView> event_to_view (wxMouseEvent &);
	TimelineContentViewList selected_views () const;
	ContentList selected_content () const;
	void maybe_snap (Time a, Time b, boost::optional<Time>& nearest_distance) const;

	FilmEditor* _film_editor;
	boost::weak_ptr<Film> _film;
	TimelineViewList _views;
	boost::shared_ptr<TimelineTimeAxisView> _time_axis_view;
	int _tracks;
	boost::optional<double> _pixels_per_time_unit;
	bool _left_down;
	wxPoint _down_point;
	boost::shared_ptr<TimelineContentView> _down_view;
	Time _down_view_position;
	bool _first_move;
	ContentMenu _menu;
	bool _snap;

	boost::signals2::scoped_connection _playlist_changed_connection;
	boost::signals2::scoped_connection _playlist_content_changed_connection;
};
