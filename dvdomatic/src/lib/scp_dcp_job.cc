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

/** @file src/scp_dcp_job.cc
 *  @brief A job to copy DCPs to a SCP-enabled server.
 */

#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <boost/filesystem.hpp>
#include <libssh/libssh.h>
#include "scp_dcp_job.h"
#include "exceptions.h"
#include "config.h"
#include "log.h"
#include "film_state.h"

using namespace std;
using namespace boost;

class SSHSession
{
public:
	SSHSession ()
		: _connected (false)
	{
		session = ssh_new ();
		if (session == 0) {
			throw NetworkError ("Could not start SSH session");
		}
	}

	int connect ()
	{
		int r = ssh_connect (session);
		if (r == 0) {
			_connected = true;
		}
		return r;
	}

	~SSHSession ()
	{
		if (_connected) {
			ssh_disconnect (session);
		}
		ssh_free (session);
	}

	ssh_session session;

private:	
	bool _connected;
};

class SSHSCP
{
public:
	SSHSCP (ssh_session s)
	{
		scp = ssh_scp_new (s, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, Config::instance()->tms_path().c_str ());
		if (!scp) {
			stringstream s;
			s << "Could not start SCP session (" << ssh_get_error (s) << ")";
			throw NetworkError (s.str ());
		}
	}

	~SSHSCP ()
	{
		ssh_scp_free (scp);
	}

	ssh_scp scp;
};


SCPDCPJob::SCPDCPJob (shared_ptr<const FilmState> s, Log* l)
	: Job (s, shared_ptr<const Options> (), l)
	, _status ("Waiting")
{

}

string
SCPDCPJob::name () const
{
	stringstream s;
	s << "Copy DCP to server";
	return s.str ();
}

void
SCPDCPJob::run ()
{
	try {
		_log->log ("SCP DCP job starting");

		SSHSession ss;

		set_status ("Connecting");

		ssh_options_set (ss.session, SSH_OPTIONS_HOST, Config::instance()->tms_ip().c_str ());
		int const port = 22;
		ssh_options_set (ss.session, SSH_OPTIONS_PORT, &port);
		
		int r = ss.connect ();
		if (r != SSH_OK) {
			stringstream s;
			s << "Could not connect to server " << Config::instance()->tms_ip() << " (" << ssh_get_error (ss.session) << ")";
			throw NetworkError (s.str ());
		}

		int const state = ssh_is_server_known (ss.session);
		if (state == SSH_SERVER_ERROR) {
			stringstream s;
			s << "SSH error (" << ssh_get_error (ss.session) << ")";
			throw NetworkError (s.str ());
		}

		r = ssh_userauth_password (ss.session, 0, Config::instance()->tms_password().c_str ());
		if (r != SSH_AUTH_SUCCESS) {
			stringstream s;
			s << "Failed to authenticate with server (" << ssh_get_error (ss.session) << ")";
			throw NetworkError (s.str ());
		}
		
		SSHSCP sc (ss.session);

		r = ssh_scp_init (sc.scp);
		if (r != SSH_OK) {
			stringstream s;
			s << "Could not start SCP session (" << ssh_get_error (ss.session) << ")";
			throw NetworkError (s.str ());
		}

		r = ssh_scp_push_directory (sc.scp, _fs->dcp_long_name.c_str(), S_IRWXU);
		if (r != SSH_OK) {
			stringstream s;
			s << "Could not create remote directory " << _fs->dcp_long_name << "(" << ssh_get_error (ss.session) << ")";
			throw NetworkError (s.str ());
		}

		string const dcp_dir = _fs->dir (_fs->dcp_long_name);

		int bytes_to_transfer = 0;
		for (filesystem::directory_iterator i = filesystem::directory_iterator (dcp_dir); i != filesystem::directory_iterator(); ++i) {
			bytes_to_transfer += filesystem::file_size (*i);
		}
		
		int buffer_size = 64 * 1024;
		char buffer[buffer_size];
		int bytes_transferred = 0;
		
		for (filesystem::directory_iterator i = filesystem::directory_iterator (dcp_dir); i != filesystem::directory_iterator(); ++i) {

			/* Aah, the sweet smell of progress */
#if BOOST_FILESYSTEM_VERSION == 3		
			string const leaf = filesystem::path(*i).leaf().generic_string ();
#else
			string const leaf = i->leaf ();
#endif

			set_status ("Copying " + leaf);

			int to_do = filesystem::file_size (*i);
			ssh_scp_push_file (sc.scp, leaf.c_str(), to_do, S_IRUSR | S_IWUSR);

			int fd = open (filesystem::path (*i).string().c_str(), O_RDONLY);
			if (fd == 0) {
				stringstream s;
				s << "Could not open " << *i << " to send";
				throw NetworkError (s.str ());
			}

			while (to_do > 0) {
				int const t = min (to_do, buffer_size);
				read (fd, buffer, t);
				r = ssh_scp_write (sc.scp, buffer, t);
				if (r != SSH_OK) {
					stringstream s;
					s << "Could not write to remote file (" << ssh_get_error (ss.session) << ")";
					throw NetworkError (s.str ());
				}
				to_do -= t;
				bytes_transferred += t;

				set_progress ((double) bytes_transferred / bytes_to_transfer);
			}
		}

		set_progress (1);
		set_status ("OK");
		set_state (FINISHED_OK);

	} catch (std::exception& e) {

		stringstream s;
		set_progress (1);
		set_state (FINISHED_ERROR);
		set_status (e.what ());

		s << "SCP DCP job failed (" << e.what() << ")";
		_log->log (s.str ());

		throw;
	}
}

string
SCPDCPJob::status () const
{
	boost::mutex::scoped_lock lm (_status_mutex);
	return _status;
}

void
SCPDCPJob::set_status (string s)
{
	boost::mutex::scoped_lock lm (_status_mutex);
	_status = s;
}
	
