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

#include "job_manager_view.h"
#include "job_manager.h"
#include "job.h"
#include "util.h"

using namespace std;

/* Must be called in the GUI thread */
JobManagerView::JobManagerView ()
{
	_scroller.set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
	
	_store = Gtk::TreeStore::create (_columns);
	_view.set_model (_store);
	_view.append_column ("Name", _columns.name);

	Gtk::CellRendererProgress* r = Gtk::manage (new Gtk::CellRendererProgress ());
	int const n = _view.append_column ("Progress", *r);
	Gtk::TreeViewColumn* c = _view.get_column (n - 1);
	c->add_attribute (r->property_value(), _columns.progress);
	c->add_attribute (r->property_text(), _columns.resolution);

	_scroller.add (_view);
	_scroller.show_all ();

	_scroller.set_size_request (-1, 150);
	
	update ();
}

/* Must be called in the GUI thread */
void
JobManagerView::update ()
{
	list<Job*> jobs = JobManager::instance()->get ();

	for (list<Job*>::iterator i = jobs.begin(); i != jobs.end(); ++i) {
		Gtk::ListStore::iterator j = _store->children().begin();
		while (j != _store->children().end()) {
			Gtk::TreeRow r = *j;
			if (r[_columns.job] == *i) {
				break;
			}
			++j;
		}

		Gtk::TreeRow r;
		if (j == _store->children().end ()) {
			j = _store->append ();
			r = *j;
			r[_columns.name] = (*i)->name ();
			r[_columns.job] = *i;
		} else {
			r = *j;
		}

		bool inform_of_finish = false;

		/* Hack to work around our lack of cross-thread
		   signalling; we tell the job to emit_finished()
		   from here (the GUI thread).
		*/
		
		r[_columns.progress] = (*i)->get_overall_progress() * 100;
		if ((*i)->finished_ok ()) {
			string const c = r[_columns.resolution];
			if (c.substr (0, 2) != "OK") {
				r[_columns.resolution] = "OK (running for " + seconds_to_hms ((*i)->elapsed_time ()) + ")";
				inform_of_finish = true;
			}
		} else if ((*i)->finished_in_error ()) {
			string const c = r[_columns.resolution];
			if (c.substr (0, 5) != "Error") {
				r[_columns.resolution] = "Error (" + (*i)->error() + ")";
				inform_of_finish = true;
			}
		}

		if (inform_of_finish) {
			(*i)->emit_finished ();
		}
	}
}
