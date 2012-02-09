#include <gtkmm.h>
#include <boost/thread.hpp>
#include "film_editor.h"
#include "progress_dialog.h"
#include "format.h"
#include "film.h"
#include "progress.h"

using namespace std;
using namespace Gtk;

FilmEditor::FilmEditor (Film* f)
	: _film (f)
	, _update_thumbs_button ("Update Thumbs")
	, _save_metadata_button ("Save Metadata")
	, _inhibit_model_to_view (false)
	, _inhibit_view_to_model (false)
{
	_vbox.set_border_width (12);
	_vbox.set_spacing (12);
	
	Table* t = manage (new Table);
	
	t->set_row_spacings (4);
	t->set_col_spacings (12);

	vector<Format*> fmt = Format::get_all ();
	for (vector<Format*>::iterator i = fmt.begin(); i != fmt.end(); ++i) {
		_format.append_text ((*i)->name ());
	}

	_left_crop.set_range (0, 1024);
	_left_crop.set_increments (1, 16);
	_left_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::crop_changed));

	_right_crop.set_range (0, 1024);
	_right_crop.set_increments (1, 16);
	_right_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::crop_changed));

	_top_crop.set_range (0, 1024);
	_top_crop.set_increments (1, 16);
	_top_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::crop_changed));

	_bottom_crop.set_range (0, 1024);
	_bottom_crop.set_increments (1, 16);
	_bottom_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::crop_changed));

	_original_size.set_alignment (0, 0.5);
	_frames_per_second.set_alignment (0, 0.5);
	
	int n = 0;
	t->attach (left_aligned_label ("Directory"), 0, 1, n, n + 1);
	t->attach (_directory, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Name"), 0, 1, n, n + 1);
	t->attach (_name, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Format"), 0, 1, n, n + 1);
	t->attach (_format, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Content"), 0, 1, n, n + 1);
	t->attach (_content, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Left Crop"), 0, 1, n, n + 1);
	t->attach (_left_crop, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Right Crop"), 0, 1, n, n + 1);
	t->attach (_right_crop, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Top Crop"), 0, 1, n, n + 1);
	t->attach (_top_crop, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Bottom Crop"), 0, 1, n, n + 1);
	t->attach (_bottom_crop, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Original Size"), 0, 1, n, n + 1);
	t->attach (_original_size, 1, 2, n, n + 1);
	++n;
	t->attach (left_aligned_label ("Frames Per Second"), 0, 1, n, n + 1);
	t->attach (_frames_per_second, 1, 2, n, n + 1);
	++n;

	t->show_all ();
	_vbox.pack_start (*t, false, false);

	HBox* h = manage (new HBox);
	h->set_spacing (12);
	h->pack_start (_update_thumbs_button, false, false);
	h->pack_start (_save_metadata_button, false, false);
	_update_thumbs_button.signal_clicked().connect (sigc::mem_fun (*this, &FilmEditor::update_thumbs_clicked));
	_save_metadata_button.signal_clicked().connect (sigc::mem_fun (*this, &FilmEditor::save_metadata_clicked));
	_vbox.pack_start (*h, false, false);
	
	model_to_view ();
}

void
FilmEditor::model_to_view ()
{
	if (_inhibit_model_to_view) {
		return;
	}
	
	_inhibit_view_to_model = true;
	
	_directory.set_text (_film->directory ());
	_name.set_text (_film->name ());
	_format.set_active (Format::get_as_index (_film->format ()));
	_content.set_filename (_film->content ());
	_left_crop.set_value (_film->left_crop ());
	_right_crop.set_value (_film->right_crop ());
	_top_crop.set_value (_film->top_crop ());
	_bottom_crop.set_value (_film->bottom_crop ());
	stringstream os;
	os << _film->width() << " x " << _film->height();
	_original_size.set_text (os.str ());
	stringstream fps;
	fps << _film->frames_per_second ();
	_frames_per_second.set_text (fps.str ());

	_inhibit_view_to_model = false;
}

void
FilmEditor::view_to_model ()
{
	if (_inhibit_view_to_model) {
		return;
	}
	
	_inhibit_model_to_view = true;
	
	_film->set_name (_name.get_text ());
	_film->set_format (Format::get_from_index (_format.get_active ()));
	_film->set_content (_content.get_filename ());
	_film->set_left_crop (_left_crop.get_value ());
	_film->set_right_crop (_right_crop.get_value ());
	_film->set_top_crop (_top_crop.get_value ());
	_film->set_bottom_crop (_bottom_crop.get_value ());

	_inhibit_model_to_view = false;
}

Widget&
FilmEditor::get_widget ()
{
	return _vbox;
}

Label&
FilmEditor::left_aligned_label (string const & t) const
{
	Label* l = manage (new Label (t));
	l->set_alignment (0, 0.5);
	return *l;
}

void
FilmEditor::crop_changed ()
{
	view_to_model ();
}

void
FilmEditor::update_thumbs_clicked ()
{
	Progress progress;
	boost::thread (boost::bind (&Film::update_thumbs_non_gui, _film, &progress));

	ProgressDialog d (&progress, "Updating Thumbnails...");
	d.spin ();

	_film->update_thumbs_gui ();
}

void
FilmEditor::save_metadata_clicked ()
{
	view_to_model ();
	_film->write_metadata ();
}
