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

/** @file src/scaler.h
 *  @brief A class to describe one of FFmpeg's software scalers.
 */

#ifndef DVDOMATIC_SCALER_H
#define DVDOMATIC_SCALER_H

#include <string>
#include <vector>

/** @class Scaler
 *  @brief Class to describe one of FFmpeg's software scalers
 */
class Scaler
{
public:
	Scaler (int f, int m, std::string i, std::string n);

	/** @return id used for calls to FFmpeg's pp_postprocess */
	int ffmpeg_id () const {
		return _ffmpeg_id;
	}

	/** @return number to use on an mplayer command line */
	int mplayer_id () const {
		return _mplayer_id;
	}

	/** @return id for our use */
	std::string id () const {
		return _id;
	}

	/** @return user-visible name for this scaler */
	std::string name () const {
		return _name;
	}
	
	static std::vector<Scaler const *> get_all ();
	static void setup_scalers ();
	static Scaler const * get_from_id (std::string id);
	static Scaler const * get_from_index (int);
	static int get_as_index (Scaler const *);

private:

	/** id used for calls to FFmpeg's pp_postprocess */
	int _ffmpeg_id;
	int _mplayer_id;
	/** id for our use */
	std::string _id;
	/** user-visible name for this scaler */
	std::string _name;

	/** sll available scalers */
	static std::vector<Scaler const *> _scalers;
};

#endif
