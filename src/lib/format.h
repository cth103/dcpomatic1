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
 *  @brief Classes to describe a format (aspect ratio) that a Film should
 *  be shown in.
 */

#include <string>
#include <vector>
#include "util.h"

class Film;

class Format
{
public:
	Format (libdcp::Size dcp, std::string id, std::string n, std::string d)
		: _dcp_size (dcp)
		, _id (id)
		, _nickname (n)
		, _dci_name (d)
	{}

	/** @return the ratio of the container (including any padding) */
	float container_ratio () const;

	int dcp_padding (boost::shared_ptr<const Film> f) const;

	/** @return size in pixels of the images that we should
	 *  put in a DCP for this ratio.  This size will not correspond
	 *  to the ratio when we are doing things like 16:9 in a Flat frame.
	 */
	libdcp::Size dcp_size () const {
		return _dcp_size;
	}

	std::string id () const {
		return _id;
	}

	/** @return Full name to present to the user */
	virtual std::string name () const = 0;

	/** @return Nickname (e.g. Flat, Scope) */
	std::string nickname () const {
		return _nickname;
	}

	std::string dci_name () const {
		return _dci_name;
	}

	std::string as_metadata () const;

	static Format const * from_nickname (std::string n);
	static Format const * from_metadata (std::string m);
	static Format const * from_id (std::string i);
	static std::vector<Format const *> all ();
	static void setup_formats ();

protected:	
        /** @return the ratio */
	virtual float ratio (boost::shared_ptr<const Film> f) const = 0;

	/** libdcp::Size in pixels of the images that we should
	 *  put in a DCP for this ratio.  This size will not correspond
	 *  to the ratio when we are doing things like 16:9 in a Flat frame.
	 */
	libdcp::Size _dcp_size;
	/** id for use in metadata */
	std::string _id;
	/** nickname (e.g. Flat, Scope) */
	std::string _nickname;
	std::string _dci_name;

private:	
	/** all available formats */
	static std::vector<Format const *> _formats;
};

/** @class FixedFormat
 *  @brief Class to describe a format whose ratio is fixed regardless
 *  of source size.
 */
class FixedFormat : public Format
{
public:
	FixedFormat (float, libdcp::Size, std::string, std::string, std::string);

	float ratio (boost::shared_ptr<const Film>) const {
		return _ratio;
	}

	std::string name () const;
	
private:

	float _ratio;
};

class VariableFormat : public Format
{
public:
	VariableFormat (libdcp::Size, std::string, std::string, std::string);

	float ratio (boost::shared_ptr<const Film> f) const;

	std::string name () const;
};