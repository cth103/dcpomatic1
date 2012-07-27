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

/** @file src/job_manager_view.cc
 *  @brief Class generating a GTK widget to show the progress of jobs.
 */

#include "lib/job_manager.h"
#include "lib/job.h"
#include "lib/util.h"
#include "lib/exceptions.h"
#include "job_manager_view.h"
#include "wx_util.h"

using namespace std;
using namespace boost;

/** Must be called in the GUI thread */
JobManagerView::JobManagerView (wxWindow* parent)
	: wxScrolledWindow (parent)
{
	_panel = new wxPanel (this);
	wxSizer* sizer = new wxBoxSizer (wxVERTICAL);
	sizer->Add (_panel, 1, wxEXPAND);
	SetSizer (sizer);
	
	_table = new wxFlexGridSizer (3, 6, 6);
	_table->AddGrowableCol (1, 1);
	_panel->SetSizer (_table);

	SetScrollRate (0, 32);

	Connect (wxID_ANY, wxEVT_TIMER, wxTimerEventHandler (JobManagerView::periodic), 0, this);
	_timer.reset (new wxTimer (this));
	_timer->Start (1000);

	update ();
}

void
JobManagerView::periodic (wxTimerEvent &)
{
	update ();
}

/** Update the view by examining the state of each job.
 *  Must be called in the GUI thread.
 */
void
JobManagerView::update ()
{
	list<shared_ptr<Job> > jobs = JobManager::instance()->get ();

	for (list<shared_ptr<Job> >::iterator i = jobs.begin(); i != jobs.end(); ++i) {
		
		if (_job_records.find (*i) == _job_records.end ()) {
			add_label_to_sizer (_table, _panel, (*i)->name ());
			JobRecord r;
			r.gauge = new wxGauge (_panel, wxID_ANY, 100);
			_table->Add (r.gauge, 1, wxEXPAND | wxLEFT | wxRIGHT);
			r.informed_of_finish = false;
			r.message = add_label_to_sizer (_table, _panel, "", 1);
			
			_job_records[*i] = r;
		}

		string const st = (*i)->status ();

		if (!(*i)->finished ()) {
			float const p = (*i)->overall_progress ();
			if (p >= 0) {
				_job_records[*i].message->SetLabel (std_to_wx (st));
				_job_records[*i].gauge->SetValue (p * 100);
			} else {
				_job_records[*i].message->SetLabel (wxT ("Running"));
				_job_records[*i].gauge->Pulse ();
			}
		}
		
		/* Hack to work around our lack of cross-thread
		   signalling; we tell the job to emit_finished()
		   from here (the GUI thread).
		*/
		
		if ((*i)->finished () && !_job_records[*i].informed_of_finish) {
			_job_records[*i].gauge->SetValue (100);
			_job_records[*i].message->SetLabel (std_to_wx (st));

			try {
				(*i)->emit_finished ();
			} catch (OpenFileError& e) {
				stringstream s;
				s << "Error: " << e.what();
				error_dialog (this, s.str ());
			}
			
			_job_records[*i].informed_of_finish = true;
		}
	}

	_table->Layout ();
	FitInside ();
}
