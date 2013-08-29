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

#ifndef DCPOMATIC_CONTENT_MENU_H
#define DCPOMATIC_CONTENT_MENU_H

#include <wx/wx.h>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include "lib/types.h"

class Film;

class ContentMenu
{
public:
	ContentMenu (boost::shared_ptr<Film>, wxWindow *);
	~ContentMenu ();

	void popup (ContentList, wxPoint);

private:
	void repeat (wxCommandEvent &);
	void remove (wxCommandEvent &);
	
	wxMenu* _menu;
	boost::weak_ptr<Film> _film;
	wxWindow* _parent;
	ContentList _content;
};

#endif