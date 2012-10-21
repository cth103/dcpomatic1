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
			throw NetworkError (String::compose ("Could not start SCP session (%1)", ssh_get_error (s)));
		}
	}

	~SSHSCP ()
	{
		ssh_scp_free (scp);
	}

	ssh_scp scp;
};


SCPDCPJob::SCPDCPJob (shared_ptr<const FilmState> s, Log* l, shared_ptr<Job> req)
	: Job (s, l, req)
	, _status ("Waiting")
{

}

string
SCPDCPJob::name () const
{
	return "Copy DCP to TMS";
}

void
SCPDCPJob::run ()
{
	_log->log ("SCP DCP job starting");
	
	SSHSession ss;
	
	set_status ("connecting");
	
	ssh_options_set (ss.session, SSH_OPTIONS_HOST, Config::instance()->tms_ip().c_str ());
	ssh_options_set (ss.session, SSH_OPTIONS_USER, Config::instance()->tms_user().c_str ());
	int const port = 22;
	ssh_options_set (ss.session, SSH_OPTIONS_PORT, &port);
	
	int r = ss.connect ();
	if (r != SSH_OK) {
		throw NetworkError (String::compose ("Could not connect to server %1 (%2)", Config::instance()->tms_ip(), ssh_get_error (ss.session)));
	}
	
	int const state = ssh_is_server_known (ss.session);
	if (state == SSH_SERVER_ERROR) {
		throw NetworkError (String::compose ("SSH error (%1)", ssh_get_error (ss.session)));
	}
	
	r = ssh_userauth_password (ss.session, 0, Config::instance()->tms_password().c_str ());
	if (r != SSH_AUTH_SUCCESS) {
		throw NetworkError (String::compose ("Failed to authenticate with server (%1)", ssh_get_error (ss.session)));
	}
	
	SSHSCP sc (ss.session);
	
	r = ssh_scp_init (sc.scp);
	if (r != SSH_OK) {
		throw NetworkError (String::compose ("Could not start SCP session (%1)", ssh_get_error (ss.session)));
	}
	
	r = ssh_scp_push_directory (sc.scp, _fs->dcp_name().c_str(), S_IRWXU);
	if (r != SSH_OK) {
		throw NetworkError (String::compose ("Could not create remote directory %1 (%2)", _fs->dcp_name(), ssh_get_error (ss.session)));
	}
	
	string const dcp_dir = _fs->dir (_fs->dcp_name());
	
	boost::uintmax_t bytes_to_transfer = 0;
	for (filesystem::directory_iterator i = filesystem::directory_iterator (dcp_dir); i != filesystem::directory_iterator(); ++i) {
		bytes_to_transfer += filesystem::file_size (*i);
	}
	
	boost::uintmax_t buffer_size = 64 * 1024;
	char buffer[buffer_size];
	boost::uintmax_t bytes_transferred = 0;
	
	for (filesystem::directory_iterator i = filesystem::directory_iterator (dcp_dir); i != filesystem::directory_iterator(); ++i) {
		
		/* Aah, the sweet smell of progress */
#if BOOST_FILESYSTEM_VERSION == 3		
		string const leaf = filesystem::path(*i).leaf().generic_string ();
#else
		string const leaf = i->leaf ();
#endif
		
		set_status ("copying " + leaf);
		
		boost::uintmax_t to_do = filesystem::file_size (*i);
		ssh_scp_push_file (sc.scp, leaf.c_str(), to_do, S_IRUSR | S_IWUSR);

		FILE* f = fopen (filesystem::path (*i).string().c_str(), "rb");
		if (f == 0) {
			throw NetworkError (String::compose ("Could not open %1 to send", *i));
		}

		while (to_do > 0) {
			int const t = min (to_do, buffer_size);
			size_t const read = fread (buffer, 1, t, f);
			if (read != size_t (t)) {
				throw ReadFileError (filesystem::path (*i).string());
			}
			
			r = ssh_scp_write (sc.scp, buffer, t);
			if (r != SSH_OK) {
				throw NetworkError (String::compose ("Could not write to remote file (%1)", ssh_get_error (ss.session)));
			}
			to_do -= t;
			bytes_transferred += t;
			
			set_progress ((double) bytes_transferred / bytes_to_transfer);
		}

		fclose (f);
	}
	
	set_progress (1);
	set_status ("");
	set_state (FINISHED_OK);
}

string
SCPDCPJob::status () const
{
	boost::mutex::scoped_lock lm (_status_mutex);
	stringstream s;
	s << Job::status ();
	if (!_status.empty ()) {
		s << "; " << _status;
	}
	return s.str ();
}

void
SCPDCPJob::set_status (string s)
{
	boost::mutex::scoped_lock lm (_status_mutex);
	_status = s;
}
	
