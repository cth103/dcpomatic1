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

#include <stdint.h>
#include <cstring>
#include <algorithm>
#include <iostream>
#include "delay_line.h"

using namespace std;

DelayLine::DelayLine (int d)
	: _delay (d)
	, _buffer (0)
	, _negative_delay_remaining (0)
{
	if (d > 0) {
		_buffer = new uint8_t[d];
		memset (_buffer, 0, d);
	} else if (d < 0) {
		_negative_delay_remaining = -d;
	}
}

DelayLine::~DelayLine ()
{
	delete[] _buffer;
}

int
DelayLine::feed (uint8_t* data, int size)
{
	int available = size;

	if (_delay > 0) {
		
		/* Copy the input data */
		uint8_t input[size];
		memcpy (input, data, size);

		int to_do = size;

		/* Write some of our buffer to the output */
		int const from_buffer = min (to_do, _delay);
		memcpy (data, _buffer, from_buffer);
		to_do -= from_buffer;

		/* Write some of the input to the output */
		int const from_input = min (to_do, size);
		memcpy (data + from_buffer, input, from_input);

		int const left_in_buffer = _delay - from_buffer;
		
		/* Shuffle our buffer down */
		memmove (_buffer, _buffer + from_buffer, left_in_buffer);

		/* Copy remaining input data to our buffer */
		memcpy (_buffer + left_in_buffer, input + from_input, size - from_input);

	} else if (_delay < 0) {

		int const to_do = min (size, _negative_delay_remaining);
		available = size - to_do;
		memmove (data, data + to_do, available);
		_negative_delay_remaining -= to_do;

	}

	return available;
}

void
DelayLine::get_remaining (uint8_t* buffer)
{
	memset (buffer, 0, -_delay);
}
