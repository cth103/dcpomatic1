#include <gtkmm.h>
#include <boost/thread.hpp>
#include "film_editor.h"
#include "format.h"
#include "film.h"
#include "progress.h"
#include "demux_job.h"
#include "job_manager.h"

using namespace std;
using namespace Gtk;

FilmEditor::FilmEditor (Film* f)
	: _film (f)
	, _update_thumbs_button ("Update Thumbs")
	, _save_metadata_button ("Save Metadata")
	, _demux_button ("Demux")
{
	_vbox.set_border_width (12);
	_vbox.set_spacing (12);
	
	Table* t = manage (new Table);
	
	t->set_row_spacings (4);
	t->set_col_spacings (12);

	/* Set up our widgets and connect to them to find out when they change */

	vector<Format*> fmt = Format::get_all ();
	for (vector<Format*>::iterator i = fmt.begin(); i != fmt.end(); ++i) {
		_format.append_text ((*i)->name ());
	}

	_name.signal_changed().connect (sigc::mem_fun (*this, &FilmEditor::name_changed));

	_format.signal_changed().connect (sigc::mem_fun (*this, &FilmEditor::format_changed));
	_content.signal_file_set().connect (sigc::mem_fun (*this, &FilmEditor::content_changed));

	_left_crop.set_range (0, 1024);
	_left_crop.set_increments (1, 16);
	_left_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::left_crop_changed));

	_right_crop.set_range (0, 1024);
	_right_crop.set_increments (1, 16);
	_right_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::right_crop_changed));

	_top_crop.set_range (0, 1024);
	_top_crop.set_increments (1, 16);
	_top_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::top_crop_changed));

	_bottom_crop.set_range (0, 1024);
	_bottom_crop.set_increments (1, 16);
	_bottom_crop.signal_value_changed().connect (sigc::mem_fun (*this, &FilmEditor::bottom_crop_changed));

	_original_size.set_alignment (0, 0.5);
	_frames_per_second.set_alignment (0, 0.5);

	/* Set up the table */
	
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

	/* Set up the buttons */

	HBox* h = manage (new HBox);
	h->set_spacing (12);
	h->pack_start (_update_thumbs_button, false, false);
	h->pack_start (_save_metadata_button, false, false);
	h->pack_start (_demux_button, false, false);
	_update_thumbs_button.signal_clicked().connect (sigc::mem_fun (*this, &FilmEditor::update_thumbs_clicked));
	_save_metadata_button.signal_clicked().connect (sigc::mem_fun (*this, &FilmEditor::save_metadata_clicked));
	_demux_button.signal_clicked().connect (sigc::mem_fun (*this, &FilmEditor::demux_clicked));
	_vbox.pack_start (*h, false, false);

	/* Connect to our film to find out when it changes */

	_film->Changed.connect (sigc::mem_fun (*this, &FilmEditor::film_changed));

	/* Initial set up */

	_directory.set_text (_film->directory ());
	film_changed (Film::Name);
	film_changed (Film::LeftCrop);
	film_changed (Film::FilmFormat);
	film_changed (Film::RightCrop);
	film_changed (Film::TopCrop);
	film_changed (Film::BottomCrop);
	film_changed (Film::Size);
	film_changed (Film::Content);
	film_changed (Film::FramesPerSecond);
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
FilmEditor::left_crop_changed ()
{
	_film->set_left_crop (_left_crop.get_value ());
}

void
FilmEditor::right_crop_changed ()
{
	_film->set_right_crop (_right_crop.get_value ());
}

void
FilmEditor::top_crop_changed ()
{
	_film->set_top_crop (_top_crop.get_value ());
}

void
FilmEditor::bottom_crop_changed ()
{
	_film->set_bottom_crop (_bottom_crop.get_value ());
}

void
FilmEditor::update_thumbs_clicked ()
{
//	ThumbsJob* j = new ThumbsJob (_film);
//	JobManager::instance()->add (j, boost::bind (&Film::update_thumbs_gui, _film));

	/* XXX */
//	_film->update_thumbs_gui ();
}

void
FilmEditor::save_metadata_clicked ()
{
	_film->write_metadata ();
}

void
FilmEditor::content_changed ()
{
	try {
		_film->set_content (_content.get_filename ());
	} catch (runtime_error& e) {
		_content.set_filename (_film->directory ());
		stringstream m;
		m << "Could not set content: " << e.what() << ".";
		Gtk::MessageDialog d (m.str(), false, MESSAGE_ERROR);
		d.set_title ("DVD-o-matic");
		d.run ();
	}
}

void
FilmEditor::name_changed ()
{
	_film->set_name (_name.get_text ());
}

void
FilmEditor::film_changed (Film::Property p)
{
	stringstream s;
		
	switch (p) {
	case Film::Content:
		_content.set_filename (_film->content ());
		break;
	case Film::FilmFormat:
		_format.set_active (Format::get_as_index (_film->format ()));
		break;
	case Film::LeftCrop:
		_left_crop.set_value (_film->left_crop ());
		break;
	case Film::RightCrop:
		_right_crop.set_value (_film->right_crop ());
		break;
	case Film::TopCrop:
		_top_crop.set_value (_film->top_crop ());
		break;
	case Film::BottomCrop:
		_bottom_crop.set_value (_film->bottom_crop ());
		break;
	case Film::Name:
		_name.set_text (_film->name ());
		break;
	case Film::FramesPerSecond:
		s << _film->frames_per_second ();
		_frames_per_second.set_text (s.str ());
		break;
	case Film::Size:
		s << _film->width() << " x " << _film->height();
		_original_size.set_text (s.str ());
		break;
	}
}

void
FilmEditor::format_changed ()
{
	_film->set_format (Format::get_from_index (_format.get_active_row_number ()));
}

void
FilmEditor::demux_clicked ()
{
	DemuxJob* j = new DemuxJob (_film);
	JobManager::instance()->add (j);
}
