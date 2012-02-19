/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>
    Taken from code Copyright (C) 2010-2011 Terrence Meiczinger

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

#include <openjpeg.h>

class FilmState;
class Options;
class Server;

class EncodedData
{
public:
	EncodedData (uint8_t* d, int s)
		: _data (d)
		, _size (s)
	{}

	virtual ~EncodedData () {}

	void send (int);
	void write (boost::shared_ptr<const Options>, int);

	uint8_t* data () const {
		return _data;
	}

	int size () const {
		return _size;
	}

protected:
	uint8_t* _data;
	int _size;
};

class LocallyEncodedData : public EncodedData
{
public:
	LocallyEncodedData (uint8_t* d, int s)
		: EncodedData (d, s)
	{}
};

class RemotelyEncodedData : public EncodedData
{
public:
	RemotelyEncodedData (int s);
	~RemotelyEncodedData ();
};

class Image
{
public:
	Image (uint8_t *, int, int, int, int);
	Image (int, int, int, int);
	virtual ~Image ();

	int * component_buffer (int) const;

	void encode_locally ();
	void encode_remotely (Server const *);
	EncodedData* encoded () {
		return _encoded;
	}

	int frame () const {
		return _frame;
	}
	
private:
	void create_openjpeg_container ();
	void write_encoded (boost::shared_ptr<const Options>, uint8_t *, int);
	
	int _frame;
	opj_image_cmptparm_t _cmptparm[3];
	opj_image* _image;
	opj_cparameters_t* _parameters;
	opj_cinfo_t* _cinfo;
	opj_cio_t* _cio;
	int _width;
	int _height;
	int _frames_per_second;
	EncodedData* _encoded;
};
