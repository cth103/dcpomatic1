/*
    Copyright (C) 2015 Carl Hetherington <cth@carlh.net>

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

#include "audio_mapping.h"

class AudioStream
{
public:
	AudioStream (int frame_rate, int channels);
	AudioStream (int frame_rate, AudioMapping mapping);
	
	void set_mapping (AudioMapping mapping);
	void set_frame_rate (int frame_rate);
	void set_channels (int channels);

	int frame_rate () const {
		return _frame_rate;
	}

	AudioMapping mapping () const {
		return _mapping;
	}
	
	int channels () const;

private:
	int _frame_rate;
	AudioMapping _mapping;
};
