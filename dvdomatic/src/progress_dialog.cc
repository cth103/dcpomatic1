#include "progress_dialog.h"
#include "progress.h"

using namespace std;
using namespace Gtk;

ProgressDialog::ProgressDialog (Progress* p, string const & t)
	: Dialog ("DVD-o-matic")
	, _progress (p)
{
	VBox* v = manage (new VBox);
	v->set_border_width (12);
	v->set_spacing (12);
	v->pack_start (*manage (new Label (t)));
	v->pack_start (_bar);
	
	get_vbox()->set_spacing (12);
	get_vbox()->add (*v);

	show_all ();
}

void
ProgressDialog::update ()
{
	float f = _progress->get_fraction ();
	if (f < 0) {
		_bar.pulse ();
	} else {
		_bar.set_fraction (f);
	}
}

void
ProgressDialog::spin ()
{
	while (!_progress->get_done ()) {
		update ();
		while (Gtk::Main::events_pending ()) {
			Gtk::Main::iteration (false);
		}
	}
}
