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
#include <boost/filesystem.hpp>
#include "film.h"
#include "image.h"
#include "lut.h"
#include "config.h"

using namespace std;

Image::Image (Film const * f, uint8_t* rgb, int w, int h, int fr)
	: _film (f)
	, _frame (fr)
{
	/* Create libopenjpeg image container */
	
	for (int i = 0; i < 3; ++i) {
		_cmptparm[i].dx = 1;
		_cmptparm[i].dy = 1;
		_cmptparm[i].w = w;
		_cmptparm[i].h = h;
		_cmptparm[i].x0 = 0;
		_cmptparm[i].y0 = 0;
		_cmptparm[i].prec = 12;
		_cmptparm[i].bpp = 12;
		_cmptparm[i].sgnd = 0;
	}

	_image = opj_image_create (3, &_cmptparm[0], CLRSPC_SRGB);
	if (_image == 0) {
		throw runtime_error ("Could not create libopenjpeg image");
	}

	_image->x0 = 0;
	_image->y0 = 0;
	_image->x1 = w;
	_image->y1 = h;

	int const size = w * h;

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

Image::~Image ()
{
	opj_image_destroy (_image);
}

void
Image::encode ()
{
	int const bw = Config::instance()->j2k_bandwidth ();

	/* Set the max image and component sizes based on frame_rate */
	int const max_cs_len = ((float) bw) / 8 / _film->frames_per_second ();
	int const max_comp_size = max_cs_len / 1.25;

	/* Set encoding parameters to default values */
	opj_cparameters_t parameters;
	opj_set_default_encoder_parameters (&parameters);

	/* Set default cinema parameters */
	parameters.tile_size_on = false;
	parameters.cp_tdx = 1;
	parameters.cp_tdy = 1;
	
	/* Tile part */
	parameters.tp_flag = 'C';
	parameters.tp_on = 1;
	
	/* Tile and Image shall be at (0,0) */
	parameters.cp_tx0 = 0;
	parameters.cp_ty0 = 0;
	parameters.image_offset_x0 = 0;
	parameters.image_offset_y0 = 0;

	/* Codeblock size = 32x32 */
	parameters.cblockw_init = 32;
	parameters.cblockh_init = 32;
	parameters.csty |= 0x01;
	
	/* The progression order shall be CPRL */
	parameters.prog_order = CPRL;
	
	/* No ROI */
	parameters.roi_compno = -1;
	
	parameters.subsampling_dx = 1;
	parameters.subsampling_dy = 1;
	
	/* 9-7 transform */
	parameters.irreversible = 1;
	
	parameters.tcp_rates[0] = 0;
	parameters.tcp_numlayers++;
	parameters.cp_disto_alloc = 1;
	parameters.cp_rsiz = CINEMA2K;
	parameters.cp_comment = strdup ("OpenDCP");
	parameters.cp_cinema = CINEMA2K_24;

	/* 3 components, so use MCT */
	parameters.tcp_mct = 1;
	
	/* set max image */
	parameters.max_comp_size = max_comp_size;
	parameters.tcp_rates[0] = ((float) (3 * _image->comps[0].w * _image->comps[0].h * _image->comps[0].prec)) / (max_cs_len * 8);

	/* get a J2K compressor handle */
	opj_cinfo_t* cinfo = opj_create_compress (CODEC_J2K);

	/* Set event manager to null (openjpeg 1.3 bug) */
	cinfo->event_mgr = 0;

	/* Setup the encoder parameters using the current image and user parameters */
	opj_setup_encoder (cinfo, &parameters, _image);

	opj_cio_t *cio = opj_cio_open ((opj_common_ptr) cinfo, 0, 0);

	int const r = opj_encode (cinfo, cio, _image, 0);
	if (r == 0) {
		opj_cio_close (cio);
		opj_destroy_compress (cinfo);
		throw runtime_error ("jpeg2000 encoding failed");
	}

	int const codestream_length = cio_tell (cio);

	FILE* f = fopen (j2k_path (_frame, true).c_str (), "wb");
	
	if (!f) {
		stringstream s;
		s << "Unable to create jpeg2000 file `" << j2k_path (_frame, true) << "' for writing";
		throw runtime_error (s.str ());
		opj_cio_close(cio);
		opj_destroy_compress(cinfo);
	}

	fwrite (cio->buffer, 1, codestream_length, f);
	fclose (f);

	/*  Rename the file from foo.j2c.tmp to foo.j2c now that it is complete */
	boost::filesystem::rename (j2k_path (_frame, true), j2k_path (_frame, false));

	/* Free openjpeg structure */
	opj_cio_close (cio);
	opj_destroy_compress (cinfo);
	
	/* Free user parameters structure */
	free (parameters.cp_comment);
	free (parameters.cp_matrice);
}

string
Image::j2k_path (int f, bool tmp) const
{
	stringstream d;
	d << "j2c/" << _film->j2k_sub_directory();
	
	stringstream s;
	s << _film->dir (d.str ()) << "/";
	s.width (8);
	s << setfill('0') << _frame << ".j2c";

	if (tmp) {
		s << ".tmp";
	}

	return s.str ();
}
