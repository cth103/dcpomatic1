/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>
    Copyright (C) 2000-2007 Paul Davis

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
#include <gtkmm.h>
extern "C" {
#include <libavcodec/avcodec.h>
}

extern std::string seconds_to_hms (int);
extern Gtk::Label & left_aligned_label (std::string const &);
extern void stacktrace (std::ostream&, int);
extern std::string audio_sample_format_to_string (AVSampleFormat);
extern AVSampleFormat audio_sample_format_from_string (std::string const &);

