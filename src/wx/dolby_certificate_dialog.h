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

#include <curl/curl.h>
#include "download_certificate_dialog.h"

class DolbyCertificateDialog : public DownloadCertificateDialog
{
public:
	DolbyCertificateDialog (wxWindow *, boost::function<void (boost::filesystem::path)>);

private:
	void download ();
	void finish_download ();
	void setup_countries ();
	void finish_setup_countries ();
	void country_selected ();
	void finish_country_selected ();
	void cinema_selected ();
	void finish_cinema_selected ();
	void serial_selected ();
	std::list<std::string> get_dir (std::string) const;

	wxChoice* _country;
	wxChoice* _cinema;
	wxChoice* _serial;
};
