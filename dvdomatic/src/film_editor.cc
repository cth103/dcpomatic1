#include <gtkmm.h>
#include "film_editor.h"
#include "format.h"
#include "film.h"

using namespace std;
using namespace Gtk;

FilmEditor::FilmEditor (Film* f)
	: _film (f)
{
	_table.set_row_spacings (4);
	_table.set_col_spacings (12);
	_table.set_border_width (12);

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
	
	int n = 0;
	_table.attach (left_aligned_label ("Directory"), 0, 1, n, n + 1);
	_table.attach (_directory, 1, 2, n, n + 1);
	++n;
	_table.attach (left_aligned_label ("Name"), 0, 1, n, n + 1);
	_table.attach (_name, 1, 2, n, n + 1);
	++n;
	_table.attach (left_aligned_label ("Format"), 0, 1, n, n + 1);
	_table.attach (_format, 1, 2, n, n + 1);
	++n;
	_table.attach (left_aligned_label ("Content"), 0, 1, n, n + 1);
	_table.attach (_content, 1, 2, n, n + 1);
	++n;
	_table.attach (left_aligned_label ("Left Crop"), 0, 1, n, n + 1);
	_table.attach (_left_crop, 1, 2, n, n + 1);
	++n;
	_table.attach (left_aligned_label ("Right Crop"), 0, 1, n, n + 1);
	_table.attach (_right_crop, 1, 2, n, n + 1);
	++n;
	_table.attach (left_aligned_label ("Top Crop"), 0, 1, n, n + 1);
	_table.attach (_top_crop, 1, 2, n, n + 1);
	++n;
	_table.attach (left_aligned_label ("Bottom Crop"), 0, 1, n, n + 1);
	_table.attach (_bottom_crop, 1, 2, n, n + 1);
	++n;

	model_to_view ();

	_table.show_all ();
}

void
FilmEditor::model_to_view ()
{
	_directory.set_text (_film->directory ());
	_name.set_text (_film->name ());
	_format.set_active (Format::get_as_index (_film->format ()));
	_content.set_filename (_film->content ());
	_left_crop.set_value (_film->left_crop ());
	_right_crop.set_value (_film->right_crop ());
	_top_crop.set_value (_film->top_crop ());
	_bottom_crop.set_value (_film->bottom_crop ());
}

void
FilmEditor::view_to_model ()
{
	_film->set_name (_directory.get_text ());
	_film->set_format (Format::get_from_index (_format.get_active ()));
	_film->set_content (_content.get_filename ());
	_film->set_left_crop (_left_crop.get_value ());
	_film->set_right_crop (_right_crop.get_value ());
	_film->set_top_crop (_top_crop.get_value ());
	_film->set_bottom_crop (_bottom_crop.get_value ());
}

Widget&
FilmEditor::get_widget ()
{
	return _table;
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

}
