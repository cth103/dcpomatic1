/*
    Copyright (C) 2013-2015 Carl Hetherington <cth@carlh.net>

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

#ifndef DCPOMATIC_COLOUR_CONVERSION_EDITOR_H
#define DCPOMATIC_COLOUR_CONVERSION_EDITOR_H

#include <boost/signals2.hpp>
#include <wx/wx.h>

class wxSpinCtrlDouble;
class ColourConversion;

class ColourConversionEditor : public wxPanel
{
public:
	ColourConversionEditor (wxWindow *);

	void set (ColourConversion);
	ColourConversion get () const;

	boost::signals2::signal<void ()> Changed;

private:
	void changed ();
	void changed (wxSpinCtrlDouble *);
	void chromaticity_changed ();
	void update_rgb_to_xyz ();
	void subhead (wxGridBagSizer* sizer, wxWindow* parent, wxString text, int& row) const;

	void set_spin_ctrl (wxSpinCtrlDouble *, double);

	std::map<wxSpinCtrlDouble*, double> _last_spin_ctrl_value;

	wxSpinCtrlDouble* _input_gamma;
	wxCheckBox* _input_gamma_linearised;
	wxChoice* _yuv_to_rgb;
	wxTextCtrl* _red_x;
	wxTextCtrl* _red_y;
	wxTextCtrl* _green_x;
	wxTextCtrl* _green_y;
	wxTextCtrl* _blue_x;
	wxTextCtrl* _blue_y;
	wxTextCtrl* _white_x;
	wxTextCtrl* _white_y;
	wxStaticText* _rgb_to_xyz[3][3];
	wxSpinCtrlDouble* _output_gamma;
};

#endif
