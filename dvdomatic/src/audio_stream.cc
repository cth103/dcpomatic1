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

#include <sstream>
#include "audio_stream.h"

using namespace std;

AudioStream::AudioStream (int s, int c)
	: _sample_rate (s)
	, _channels (c)
{

}

AudioStream::AudioStream (string const & m)
{
	stringstream s (m);
	s >> _sample_rate >> _channels;
}

string
AudioStream::get_as_metadata () const
{
	stringstream s;
	s << _sample_rate << " " << _channels;
	return s.str ();
}

string
AudioStream::get_as_description () const
{
	stringstream s;
	s << _sample_rate << "Hz, " << _channels << " channels";
	return s.str ();
}
