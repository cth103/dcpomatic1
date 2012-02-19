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

#include <stdint.h>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <boost/filesystem.hpp>
#include "film.h"
#include "image.h"
#include "lut.h"
#include "config.h"
#include "film_state.h"
#include "options.h"
#include "exceptions.h"
#include "server.h"
#include "util.h"

using namespace std;
using namespace boost;

/** Construct an empty image */
Image::Image (int f, int w, int h, int fps)
	: _frame (f)
	, _parameters (0)
	, _cinfo (0)
	, _cio (0)
	, _width (w)
	, _height (h)
	, _frames_per_second (fps)
	, _encoded (0)
{
	create_openjpeg_container ();
}

/** Construct an Image from an RGB buffer */
Image::Image (uint8_t* rgb, int f, int w, int h, int fps)
	: _frame (f)
	, _parameters (0)
	, _cinfo (0)
	, _cio (0)
	, _width (w)
	, _height (h)
	, _frames_per_second (fps)
	, _encoded (0)
{
	create_openjpeg_container ();

	int const size = _width * _height;

	struct {
		float r, g, b;
	} s;

	struct {
		float x, y, z;
	} d;

	/* Copy our RGB into it, converting to XYZ in the process */

	int const lut_index = Config::instance()->colour_lut_index ();
	
	uint8_t* p = rgb;
	for (int i = 0; i < size; ++i) {
		/* In gamma LUT (converting 8-bit input to 12-bit) */
		s.r = lut_in[lut_index][*p++ << 4];
		s.g = lut_in[lut_index][*p++ << 4];
		s.b = lut_in[lut_index][*p++ << 4];

		/* RGB to XYZ Matrix */
		d.x = ((s.r * color_matrix[lut_index][0][0]) + (s.g * color_matrix[lut_index][0][1]) + (s.b * color_matrix[lut_index][0][2]));
		d.y = ((s.r * color_matrix[lut_index][1][0]) + (s.g * color_matrix[lut_index][1][1]) + (s.b * color_matrix[lut_index][1][2]));
		d.z = ((s.r * color_matrix[lut_index][2][0]) + (s.g * color_matrix[lut_index][2][1]) + (s.b * color_matrix[lut_index][2][2]));

		/* DCI companding */
		d.x = d.x * DCI_COEFFICENT * (DCI_LUT_SIZE - 1);
		d.y = d.y * DCI_COEFFICENT * (DCI_LUT_SIZE - 1);
		d.z = d.z * DCI_COEFFICENT * (DCI_LUT_SIZE - 1);
		
		/* Out gamma LUT */
		_image->comps[0].data[i] = lut_out[LO_DCI][(int) d.x];
		_image->comps[1].data[i] = lut_out[LO_DCI][(int) d.y];
		_image->comps[2].data[i] = lut_out[LO_DCI][(int) d.z];
	}
}

void
Image::create_openjpeg_container ()
{
	for (int i = 0; i < 3; ++i) {
		_cmptparm[i].dx = 1;
		_cmptparm[i].dy = 1;
		_cmptparm[i].w = _width;
		_cmptparm[i].h = _height;
		_cmptparm[i].x0 = 0;
		_cmptparm[i].y0 = 0;
		_cmptparm[i].prec = 12;
		_cmptparm[i].bpp = 12;
		_cmptparm[i].sgnd = 0;
	}

	_image = opj_image_create (3, &_cmptparm[0], CLRSPC_SRGB);
	if (_image == 0) {
		throw EncodeError ("could not create libopenjpeg image");
	}

	_image->x0 = 0;
	_image->y0 = 0;
	_image->x1 = _width;
	_image->y1 = _height;
}

Image::~Image ()
{
	opj_image_destroy (_image);

	if (_cio) {
		opj_cio_close (_cio);
	}

	if (_cinfo) {
		opj_destroy_compress (_cinfo);
	}

	if (_parameters) {
		free (_parameters->cp_comment);
		free (_parameters->cp_matrice);
	}
	
	delete _parameters;

	delete _encoded;
}

void
Image::encode_locally ()
{
	int const bw = Config::instance()->j2k_bandwidth ();

	/* Set the max image and component sizes based on frame_rate */
	int const max_cs_len = ((float) bw) / 8 / _frames_per_second;
	int const max_comp_size = max_cs_len / 1.25;

	/* Set encoding parameters to default values */
	_parameters = new opj_cparameters_t;
	opj_set_default_encoder_parameters (_parameters);

	/* Set default cinema parameters */
	_parameters->tile_size_on = false;
	_parameters->cp_tdx = 1;
	_parameters->cp_tdy = 1;
	
	/* Tile part */
	_parameters->tp_flag = 'C';
	_parameters->tp_on = 1;
	
	/* Tile and Image shall be at (0,0) */
	_parameters->cp_tx0 = 0;
	_parameters->cp_ty0 = 0;
	_parameters->image_offset_x0 = 0;
	_parameters->image_offset_y0 = 0;

	/* Codeblock size = 32x32 */
	_parameters->cblockw_init = 32;
	_parameters->cblockh_init = 32;
	_parameters->csty |= 0x01;
	
	/* The progression order shall be CPRL */
	_parameters->prog_order = CPRL;
	
	/* No ROI */
	_parameters->roi_compno = -1;
	
	_parameters->subsampling_dx = 1;
	_parameters->subsampling_dy = 1;
	
	/* 9-7 transform */
	_parameters->irreversible = 1;
	
	_parameters->tcp_rates[0] = 0;
	_parameters->tcp_numlayers++;
	_parameters->cp_disto_alloc = 1;
	_parameters->cp_rsiz = CINEMA2K;
	_parameters->cp_comment = strdup ("OpenDCP");
	_parameters->cp_cinema = CINEMA2K_24;

	/* 3 components, so use MCT */
	_parameters->tcp_mct = 1;
	
	/* set max image */
	_parameters->max_comp_size = max_comp_size;
	_parameters->tcp_rates[0] = ((float) (3 * _image->comps[0].w * _image->comps[0].h * _image->comps[0].prec)) / (max_cs_len * 8);

	/* get a J2K compressor handle */
	_cinfo = opj_create_compress (CODEC_J2K);

	/* Set event manager to null (openjpeg 1.3 bug) */
	_cinfo->event_mgr = 0;

	/* Setup the encoder parameters using the current image and user parameters */
	opj_setup_encoder (_cinfo, _parameters, _image);

	_cio = opj_cio_open ((opj_common_ptr) _cinfo, 0, 0);

	int const r = opj_encode (_cinfo, _cio, _image, 0);
	if (r == 0) {
		throw EncodeError ("jpeg2000 encoding failed");
	}

	_encoded = new LocallyEncodedData (_cio->buffer, cio_tell (_cio));
}

void
Image::encode_remotely (Server const * serv)
{
	int const fd = socket (AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		throw NetworkError ("could not create socket");
	}

	struct hostent* server = gethostbyname (serv->host_name().c_str ());
	if (server == 0) {
		throw NetworkError ("gethostbyname failed");
	}

	struct sockaddr_in server_address;
	memset (&server_address, 0, sizeof (server_address));
	server_address.sin_family = AF_INET;
	memcpy (&server_address.sin_addr.s_addr, server->h_addr, server->h_length);
	server_address.sin_port = htons (Config::instance()->server_port ());
	if (connect (fd, (struct sockaddr *) &server_address, sizeof (server_address)) < 0) {
		stringstream s;
		s << "could not connect (" << strerror (errno) << ")";
		throw NetworkError (s.str());
	}

	stringstream s;
	s << "encode " << _frame << " " << _width << " " << _height << " " << _frames_per_second;
	fd_write (fd, (uint8_t *) s.str().c_str(), s.str().length() + 1);

	for (int i = 0; i < 3; ++i) {
		fd_write (fd, (uint8_t *) _image->comps[i].data, _width * _height * sizeof (int));
	}

	SocketReader reader (fd);

	char buffer[32];
	reader.read_indefinite ((uint8_t *) buffer, sizeof (buffer));
	reader.consume (strlen (buffer) + 1);

	if (strcmp (buffer, "OK") != 0) {
		throw NetworkError ("bad reply from server");
	}

	reader.read_indefinite ((uint8_t *) buffer, sizeof (buffer));
	reader.consume (strlen (buffer) + 1);
	_encoded = new RemotelyEncodedData (atoi (buffer));

	/* now read the rest */
	reader.read_definite_and_consume (_encoded->data(), _encoded->size());

	close (fd);
}

int *
Image::component_buffer (int n) const
{
	return _image->comps[n].data;
}


void
EncodedData::write (shared_ptr<const Options> opt, int frame)
{
	string const tmp_j2k = opt->frame_out_path (frame, true);

	FILE* f = fopen (tmp_j2k.c_str (), "wb");
	
	if (!f) {
		throw WriteFileError (tmp_j2k);
	}

	fwrite (_data, 1, _size, f);
	fclose (f);

	/* Rename the file from foo.j2c.tmp to foo.j2c now that it is complete */
	filesystem::rename (tmp_j2k, opt->frame_out_path (frame, false));
}


void
EncodedData::send (int fd)
{
	stringstream s;
	s << _size;
	fd_write (fd, (uint8_t *) s.str().c_str(), s.str().length() + 1);
	fd_write (fd, _data, _size);
}

RemotelyEncodedData::RemotelyEncodedData (int s)
	: EncodedData (new uint8_t[s], s)
{

}

RemotelyEncodedData::~RemotelyEncodedData ()
{
	delete[] _data;
}
