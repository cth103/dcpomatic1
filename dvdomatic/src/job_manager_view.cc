#include "job_manager_view.h"
#include "job_manager.h"
#include "job.h"

using namespace std;

JobManagerView::JobManagerView ()
{
	_store = Gtk::TreeStore::create (_columns);
	_view.set_model (_store);
	_view.append_column ("Name", _columns.name);

	Gtk::CellRendererProgress* r = Gtk::manage (new Gtk::CellRendererProgress ());
	int const n = _view.append_column ("Progress", *r);
	Gtk::TreeViewColumn* c = _view.get_column (n - 1);
	c->add_attribute (r->property_value(), _columns.progress);
	c->add_attribute (r->property_text(), _columns.resolution);
	
	update ();
}

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

		r[_columns.progress] = (*i)->progress() * 100;
		if ((*i)->finished_ok ()) {
			r[_columns.resolution] = "OK";
		} else if ((*i)->finished_in_error ()) {
			r[_columns.resolution] = "Error";
		}
	}
}
