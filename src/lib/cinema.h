/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

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

/** @file  src/lib/cinema.h
 *  @brief Screen and Cinema classes.
 */

#include <boost/enable_shared_from_this.hpp>
#include <dcp/certificates.h>

class Cinema;

namespace cxml {
	class Node;
}

/** @class Screen
 *  @brief A representation of a Screen for KDM generation.
 *
 *  This is the name of the screen and the certificate of its
 *  server.
 */
class Screen
{
public:
	Screen (std::string const & n, boost::shared_ptr<dcp::Certificate> cert)
		: name (n)
		, certificate (cert)
	{}

	Screen (boost::shared_ptr<const cxml::Node>);

	void as_xml (xmlpp::Element *) const;
	
	boost::shared_ptr<Cinema> cinema;
	std::string name;
	boost::shared_ptr<dcp::Certificate> certificate;
};

/** @class Cinema
 *  @brief A description of a Cinema for KDM generation.
 *
 *  This is a cinema name, contact email address and a list of
 *  Screen objects.
 */
class Cinema : public boost::enable_shared_from_this<Cinema>
{
public:
	Cinema (std::string const & n, std::string const & e)
		: name (n)
		, email (e)
	{}

	Cinema (boost::shared_ptr<const cxml::Node>);

	void read_screens (boost::shared_ptr<const cxml::Node>);

	void as_xml (xmlpp::Element *) const;

	void add_screen (boost::shared_ptr<Screen>);
	void remove_screen (boost::shared_ptr<Screen>);
	
	std::string name;
	std::string email;
	std::list<boost::shared_ptr<Screen> > screens () const {
		return _screens;
	}

private:	
	std::list<boost::shared_ptr<Screen> > _screens;
};
