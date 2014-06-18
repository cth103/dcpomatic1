/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#ifndef DCPOMATIC_SUBRIP_H
#define DCPOMATIC_SUBRIP_H

#include "subrip_subtitle.h"

class SubRipContent;
class subrip_time_test;
class subrip_coordinate_test;
class subrip_content_test;
class subrip_parse_test;

class SubRip
{
public:
	SubRip (boost::shared_ptr<const SubRipContent>);

	ContentTime length () const;

protected:
	std::vector<SubRipSubtitle> _subtitles;
	
private:
	friend class subrip_time_test;
	friend class subrip_coordinate_test;
	friend class subrip_content_test;
	friend class subrip_parse_test;
	
	static ContentTime convert_time (std::string);
	static int convert_coordinate (std::string);
	static std::list<SubRipSubtitlePiece> convert_content (std::list<std::string>);
	static void maybe_content (std::list<SubRipSubtitlePiece> &, SubRipSubtitlePiece &);
};

#endif