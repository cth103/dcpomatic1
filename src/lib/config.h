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

/** @file src/config.h
 *  @brief Class holding configuration.
 */

#ifndef DVDOMATIC_CONFIG_H
#define DVDOMATIC_CONFIG_H

#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>

class ServerDescription;
class Screen;
class Scaler;
class Filter;
class SoundProcessor;

/** @class Config
 *  @brief A singleton class holding configuration.
 */
class Config
{
public:

	/** @return number of threads to use for J2K encoding on the local machine */
	int num_local_encoding_threads () const {
		return _num_local_encoding_threads;
	}

	/** @return port to use for J2K encoding servers */
	int server_port () const {
		return _server_port;
	}

	/** @return index of colour LUT to use when converting RGB to XYZ.
	 *  0: sRGB
	 *  1: Rec 709
	 */
	int colour_lut_index () const {
		return _colour_lut_index;
	}

	/** @return bandwidth for J2K files in bits per second */
	int j2k_bandwidth () const {
		return _j2k_bandwidth;
	}

	/** @return J2K encoding servers to use */
	std::vector<ServerDescription*> servers () const {
		return _servers;
	}

	std::vector<boost::shared_ptr<Screen> > screens () const {
		return _screens;
	}

	Scaler const * reference_scaler () const {
		return _reference_scaler;
	}

	std::vector<Filter const *> reference_filters () const {
		return _reference_filters;
	}

	/** @return The IP address of a TMS that we can copy DCPs to */
	std::string tms_ip () const {
		return _tms_ip;
	}

	/** @return The path on a TMS that we should write DCPs to */
	std::string tms_path () const {
		return _tms_path;
	}

	/** @return User name to log into the TMS with */
	std::string tms_user () const {
		return _tms_user;
	}

	/** @return Password to log into the TMS with */
	std::string tms_password () const {
		return _tms_password;
	}

	SoundProcessor const * sound_processor () const {
		return _sound_processor;
	}

	/** @param n New number of local encoding threads */
	void set_num_local_encoding_threads (int n) {
		_num_local_encoding_threads = n;
		Changed ();
	}

	/** @param p New server port */
	void set_server_port (int p) {
		_server_port = p;
		Changed ();
	}

	/** @param i New colour LUT index */
	void set_colour_lut_index (int i) {
		_colour_lut_index = i;
		Changed ();
	}

	/** @param b New J2K bandwidth */
	void set_j2k_bandwidth (int b) {
		_j2k_bandwidth = b;
		Changed ();
	}

	/** @param s New list of servers */
	void set_servers (std::vector<ServerDescription*> s) {
		_servers = s;
		Changed ();
	}

	void set_screens (std::vector<boost::shared_ptr<Screen> > s) {
		_screens = s;
		Changed ();
	}

	void set_reference_scaler (Scaler const * s) {
		_reference_scaler = s;
		Changed ();
	}
	
	void set_reference_filters (std::vector<Filter const *> const & f) {
		_reference_filters = f;
		Changed ();
	}

	/** @param i IP address of a TMS that we can copy DCPs to */
	void set_tms_ip (std::string i) {
		_tms_ip = i;
		Changed ();
	}

	/** @param p Path on a TMS that we should write DCPs to */
	void set_tms_path (std::string p) {
		_tms_path = p;
		Changed ();
	}

	/** @param u User name to log into the TMS with */
	void set_tms_user (std::string u) {
		_tms_user = u;
		Changed ();
	}

	/** @param p Password to log into the TMS with */
	void set_tms_password (std::string p) {
		_tms_password = p;
		Changed ();
	}
	
	void write () const;

	boost::signals2::signal<void()> Changed;

	static Config* instance ();

private:
	Config ();
	std::string file () const;

	/** number of threads to use for J2K encoding on the local machine */
	int _num_local_encoding_threads;
	/** port to use for J2K encoding servers */
	int _server_port;
	/** index of colour LUT to use when converting RGB to XYZ
	 *  (see colour_lut_index ())
	 */
	int _colour_lut_index;
	/** bandwidth for J2K files in bits per second */
	int _j2k_bandwidth;

	/** J2K encoding servers to use */
	std::vector<ServerDescription *> _servers;
	/** Screen definitions */
	std::vector<boost::shared_ptr<Screen> > _screens;
	/** Scaler to use for the "A" part of A/B comparisons */
	Scaler const * _reference_scaler;
	/** Filters to use for the "A" part of A/B comparisons */
	std::vector<Filter const *> _reference_filters;
	/** The IP address of a TMS that we can copy DCPs to */
	std::string _tms_ip;
	/** The path on a TMS that we should write DCPs to */
	std::string _tms_path;
	/** User name to log into the TMS with */
	std::string _tms_user;
	/** Password to log into the TMS with */
	std::string _tms_password;
	/** Our sound processor */
	SoundProcessor const * _sound_processor;

	/** Singleton instance, or 0 */
	static Config* _instance;
};

#endif
