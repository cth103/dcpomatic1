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

#include <string>
#include <vector>

class Format
{
public:
	Format (int, int, int, std::string, std::string);

	int ratio_as_integer () const {
		return _ratio;
	}

	float ratio_as_float () const {
		return _ratio / 100.0;
	}

	int dci_width () const {
		return _dci_width;
	}

	int dci_height () const {
		return _dci_height;
	}

	std::string name () const;

	std::string nickname () const {
		return _nickname;
	}

	std::string dcp_name () const {
		return _dcp_name;
	}

	std::string get_as_metadata () const;

	static Format * get_from_ratio (int);
	static Format * get_from_nickname (std::string);
	static Format * get_from_metadata (std::string);
	static Format * get_from_index (int);
	static int get_as_index (Format const *);
	static std::vector<Format*> get_all ();
	static void setup_formats ();
	
private:

	/** Ratio expressed as the actual ratio multiplied by 100 */
	int _ratio;
	int _dci_width;
	int _dci_height;
	std::string _nickname;
	std::string _dcp_name;

	static std::vector<Format *> _formats;
};

	
