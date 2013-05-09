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

/** @file src/job_manager_view.h
 *  @brief Class which is a wxPanel for showing the progress of jobs.
 */

#include <string>
#include <boost/shared_ptr.hpp>
#include <wx/wx.h>

class Job;

/** @class JobManagerView
 *  @brief Class which is a wxPanel for showing the progress of jobs.
 */
class JobManagerView : public wxScrolledWindow
{
public:
	enum Buttons {
		PAUSE = 0x1,
	};
		
	JobManagerView (wxWindow *, Buttons);

	void update ();

private:
	void periodic (wxTimerEvent &);
	void cancel_clicked (wxCommandEvent &);
	void pause_clicked (wxCommandEvent &);
	void details_clicked (wxCommandEvent &);

	boost::shared_ptr<wxTimer> _timer;
	wxPanel* _panel;
	wxFlexGridSizer* _table;
	struct JobRecord {
		wxGauge* gauge;
		wxStaticText* message;
		wxButton* cancel;
		wxButton* pause;
		wxButton* details;
		bool finalised;
		bool scroll_nudged;
	};
		
	std::map<boost::shared_ptr<Job>, JobRecord> _job_records;
	Buttons _buttons;
};
