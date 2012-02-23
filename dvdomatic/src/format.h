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

/** @file src/format.h
 *  @brief Class to describe a format (aspect ratio) that a Film should
 *  be shown in.
 */

#include <string>
#include <vector>
#include "util.h"

/** Class to describe a format (aspect ratio) that a Film should
 *  be shown in.
 */
class Format
{
public:
	Format (int, Size, std::string, std::string);

	/** @return the aspect ratio multiplied by 100
	 *  (e.g. 239 for Cinemascope 2.39:1)
	 */
	int ratio_as_integer () const {
		return _ratio;
	}

	/** @return the ratio as a floating point number */
	float ratio_as_float () const {
		return _ratio / 100.0;
	}

	/** @return size in pixels of the DCI specified size for this ratio */
	Size dci_size () const {
		return _dci_size;
	}

	/** @return Full name to present to the user */
	std::string name () const;

	/** @return Nickname (e.g. Flat, Scope) */
	std::string nickname () const {
		return _nickname;
	}

	/** @return Text to use for this format as part of a DCP name
	 *  (e.g. F, S)
	 */
	std::string dcp_name () const {
		return _dcp_name;
	}

	std::string get_as_metadata () const;

	static Format const * get_from_ratio (int);
	static Format const * get_from_nickname (std::string);
	static Format const * get_from_metadata (std::string);
	static Format const * get_from_index (int);
	static int get_as_index (Format const *);
	static std::vector<Format const *> get_all ();
	static void setup_formats ();
	
private:

	/** Ratio expressed as the actual ratio multiplied by 100 */
	int _ratio;
	/** size in pixels of the DCI specified size for this ratio */
	Size _dci_size;
	/** nickname (e.g. Flat, Scope) */
	std::string _nickname;
	/** text to use for this format as part of a DCP name (e.g. F, S) */
	std::string _dcp_name;

	/** all available formats */
	static std::vector<Format const *> _formats;
};

	
