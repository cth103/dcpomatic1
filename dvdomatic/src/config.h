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

#include <list>

class Server;

/** A singleton class holding configuration */
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
	 *  2: DC28
	 */
	int colour_lut_index () const {
		return _colour_lut_index;
	}

	/** @return bandwidth for J2K files in bits per second */
	int j2k_bandwidth () const {
		return _j2k_bandwidth;
	}

	/** @return J2K encoding servers to use */
	std::list<Server*> servers () const {
		return _servers;
	}

	/** @param n New number of local encoding threads */
	void set_num_local_encoding_threads (int n) {
		_num_local_encoding_threads = n;
	}

	/** @param p New server port */
	void set_sever_port (int p) {
		_server_port = p;
	}

	/** @param i New colour LUT index */
	void set_colour_lut_index (int i) {
		_colour_lut_index = i;
	}

	/** @param b New J2K bandwidth */
	void set_j2k_bandwidth (int b) {
		_j2k_bandwidth = b;
	}

	/** @param s New list of servers */
	void set_servers (std::list<Server*> s) {
		_servers = s;
	}

	void write () const;

	static Config* instance ();

private:
	Config ();
	std::string get_file () const;

	/** number of threads to use for J2K encoding on the local machine */
	int _num_local_encoding_threads;
	/** port to use for J2K encoding servers */
	int _server_port;
	/** index of colour LUT to use when converting RGB to XYZ
	 *  (see colour_lut_index ())
	 */
	int _colour_lut_index;
	/** bandwidth for J2K files in Mb/s */
	int _j2k_bandwidth;

	/** J2K encoding servers to use */
	std::list<Server *> _servers;

	/** Singleton instance, or 0 */
	static Config* _instance;
};

#endif
