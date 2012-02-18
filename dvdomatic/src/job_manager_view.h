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

#include <string>
#include <boost/shared_ptr.hpp>
#include <gtkmm.h>

class Job;

class JobManagerView
{
public:
	JobManagerView ();

	Gtk::Widget& get_widget () {
		return _scroller;
	}

	void update ();

private:
	Gtk::ScrolledWindow _scroller;
	Gtk::TreeView _view;
	Glib::RefPtr<Gtk::TreeStore> _store;

	class StoreColumns : public Gtk::TreeModelColumnRecord
	{
	public:
		StoreColumns ()
		{
			add (name);
			add (job);
			add (progress);
			add (progress_unknown);
			add (text);
		}

		Gtk::TreeModelColumn<std::string> name;
		Gtk::TreeModelColumn<boost::shared_ptr<Job> > job;
		Gtk::TreeModelColumn<float> progress;
		Gtk::TreeModelColumn<int> progress_unknown;
		Gtk::TreeModelColumn<std::string> text;
	};

	StoreColumns _columns;
};
