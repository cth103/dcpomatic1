/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>
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

/** @file  src/dcp_video_frame.cc
 *  @brief A single frame of video destined for a DCP.
 *
 *  Given an Image and some settings, this class knows how to encode
 *  the image to J2K either on the local host or on a remote server.
 *
 *  Objects of this class are used for the queue that we keep
 *  of images that require encoding.
 */

#include <stdint.h>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <errno.h>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <libdcp/rec709_linearised_gamma_lut.h>
#include <libdcp/srgb_linearised_gamma_lut.h>
#include <libdcp/gamma_lut.h>
#include <libdcp/xyz_frame.h>
#include <libdcp/rgb_xyz.h>
#include <libdcp/colour_matrix.h>
#include <libcxml/cxml.h>
#include "raw_convert.h"
#include "film.h"
#include "dcp_video_frame.h"
#include "config.h"
#include "exceptions.h"
#include "server.h"
#include "util.h"
#include "scaler.h"
#include "image.h"
#include "log.h"
#include "cross.h"
#include "player_video_frame.h"

#define LOG_GENERAL(...) _log->log (String::compose (__VA_ARGS__), Log::TYPE_GENERAL);

#include "i18n.h"

using std::string;
using std::cout;
using boost::shared_ptr;
using libdcp::Size;

#define DCI_COEFFICENT (48.0 / 52.37)

/** Construct a DCP video frame.
 *  @param frame Input frame.
 *  @param index Index of the frame within the DCP.
 *  @param bw J2K bandwidth to use (see Config::j2k_bandwidth ())
 *  @param l Log to write to.
 */
DCPVideoFrame::DCPVideoFrame (
	shared_ptr<const PlayerVideoFrame> frame, int index, int dcp_fps, int bw, Resolution r, shared_ptr<Log> l
	)
	: _frame (frame)
	, _index (index)
	, _frames_per_second (dcp_fps)
	, _j2k_bandwidth (bw)
	, _resolution (r)
	, _log (l)
{

}

DCPVideoFrame::DCPVideoFrame (shared_ptr<const PlayerVideoFrame> frame, shared_ptr<const cxml::Node> node, shared_ptr<Log> log)
	: _frame (frame)
	, _log (log)
{
	_index = node->number_child<int> ("Index");
	_frames_per_second = node->number_child<int> ("FramesPerSecond");
	_j2k_bandwidth = node->number_child<int> ("J2KBandwidth");
	_resolution = Resolution (node->optional_number_child<int>("Resolution").get_value_or (RESOLUTION_2K));
}

/** J2K-encode this frame on the local host.
 *  @return Encoded data.
 */
shared_ptr<EncodedData>
DCPVideoFrame::encode_locally ()
{
	shared_ptr<libdcp::XYZFrame> xyz;

	if (_frame->colour_conversion()) {
		ColourConversion conversion = _frame->colour_conversion().get ();
		shared_ptr<libdcp::LUT> in_lut;
		if (conversion.input_gamma_linearised) {
			in_lut = libdcp::SRGBLinearisedGammaLUT::cache.get (12, conversion.input_gamma);
		} else {
			in_lut = libdcp::GammaLUT::cache.get (12, conversion.input_gamma);
		}

		/* XXX: libdcp should probably use boost */

		boost::numeric::ublas::matrix<double> A = conversion.rgb_to_xyz ();

		double rgb_to_xyz[3][3];
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				rgb_to_xyz[i][j] = A (i, j);
			}
		}

		boost::numeric::ublas::matrix<double> B = conversion.bradford ();

		double bradford[3][3];
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				bradford[i][j] = B (i, j);
			}
		}

		xyz = libdcp::rgb_to_xyz (
			_frame->image(AV_PIX_FMT_RGB48LE),
			in_lut,
			libdcp::GammaLUT::cache.get (16, 1 / conversion.output_gamma),
			rgb_to_xyz,
			bradford
			);
	} else {
		xyz = libdcp::xyz_to_xyz (_frame->image (AV_PIX_FMT_RGB48LE));
	}

	/* Set the max image and component sizes based on frame_rate */
	int max_cs_len = ((float) _j2k_bandwidth) / 8 / _frames_per_second;
	if (_frame->eyes() == EYES_LEFT || _frame->eyes() == EYES_RIGHT) {
		/* In 3D we have only half the normal bandwidth per eye */
		max_cs_len /= 2;
	}
	int const max_comp_size = max_cs_len / 1.25;

	/* get a J2K compressor handle */
	opj_cinfo_t* cinfo = opj_create_compress (CODEC_J2K);
	if (cinfo == 0) {
		throw EncodeError (N_("could not create JPEG2000 encoder"));
	}

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
	parameters.cp_rsiz = _resolution == RESOLUTION_2K ? CINEMA2K : CINEMA4K;
	if (_resolution == RESOLUTION_4K) {
		parameters.numpocs = 2;
		parameters.POC[0].tile = 1;
		parameters.POC[0].resno0 = 0;
		parameters.POC[0].compno0 = 0;
		parameters.POC[0].layno1 = 1;
		parameters.POC[0].resno1 = parameters.numresolution - 1;
		parameters.POC[0].compno1 = 3;
		parameters.POC[0].prg1 = CPRL;
		parameters.POC[1].tile = 1;
		parameters.POC[1].resno0 = parameters.numresolution - 1;
		parameters.POC[1].compno0 = 0;
		parameters.POC[1].layno1 = 1;
		parameters.POC[1].resno1 = parameters.numresolution;
		parameters.POC[1].compno1 = 3;
		parameters.POC[1].prg1 = CPRL;
	}

	parameters.cp_comment = strdup (N_("DCP-o-matic"));
	parameters.cp_cinema = _resolution == RESOLUTION_2K ? CINEMA2K_24 : CINEMA4K_24;

	/* 3 components, so use MCT */
	parameters.tcp_mct = 1;

	/* set max image */
	parameters.max_comp_size = max_comp_size;
	parameters.tcp_rates[0] = ((float) (3 * xyz->size().width * xyz->size().height * 12)) / (max_cs_len * 8);

	/* Set event manager to null (openjpeg 1.3 bug) */
	cinfo->event_mgr = 0;

	/* Setup the encoder parameters using the current image and user parameters */
	opj_setup_encoder (cinfo, &parameters, xyz->opj_image ());

	opj_cio_t* cio = opj_cio_open ((opj_common_ptr) cinfo, 0, 0);
	if (cio == 0) {
		opj_destroy_compress (cinfo);
		throw EncodeError (N_("could not open JPEG2000 stream"));
	}

	int const r = opj_encode (cinfo, cio, xyz->opj_image(), 0);
	if (r == 0) {
		opj_cio_close (cio);
		opj_destroy_compress (cinfo);
		throw EncodeError (N_("JPEG2000 encoding failed"));
	}

	switch (_frame->eyes()) {
	case EYES_BOTH:
		LOG_GENERAL (N_("Finished locally-encoded frame %1 for mono"), _index);
		break;
	case EYES_LEFT:
		LOG_GENERAL (N_("Finished locally-encoded frame %1 for L"), _index);
		break;
	case EYES_RIGHT:
		LOG_GENERAL (N_("Finished locally-encoded frame %1 for R"), _index);
		break;
	default:
		break;
	}

	shared_ptr<EncodedData> enc (new LocallyEncodedData (cio->buffer, cio_tell (cio)));

	opj_cio_close (cio);
	free (parameters.cp_comment);
	opj_destroy_compress (cinfo);

	return enc;
}

/** Send this frame to a remote server for J2K encoding, then read the result.
 *  @param serv Server to send to.
 *  @return Encoded data.
 */
shared_ptr<EncodedData>
DCPVideoFrame::encode_remotely (ServerDescription serv)
{
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::resolver resolver (io_service);
	boost::asio::ip::tcp::resolver::query query (serv.host_name(), raw_convert<string> (Config::instance()->server_port_base ()));
	boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve (query);

	shared_ptr<Socket> socket (new Socket);

	socket->connect (*endpoint_iterator);

	/* Collect all XML metadata */
	xmlpp::Document doc;
	xmlpp::Element* root = doc.create_root_node ("EncodingRequest");
	root->add_child("Version")->add_child_text (raw_convert<string> (SERVER_LINK_VERSION));
	add_metadata (root);

	LOG_GENERAL (N_("Sending frame %1 to remote"), _index);

	/* Send XML metadata */
	string xml = doc.write_to_string ("UTF-8");
	socket->write (xml.length() + 1);
	socket->write ((uint8_t *) xml.c_str(), xml.length() + 1);

	/* Send binary data */
	_frame->send_binary (socket);

	/* Read the response (JPEG2000-encoded data); this blocks until the data
	   is ready and sent back.
	*/
	shared_ptr<EncodedData> e (new RemotelyEncodedData (socket->read_uint32 ()));
	socket->read (e->data(), e->size());

	LOG_GENERAL (N_("Finished remotely-encoded frame %1"), _index);

	return e;
}

void
DCPVideoFrame::add_metadata (xmlpp::Element* el) const
{
	el->add_child("Index")->add_child_text (raw_convert<string> (_index));
	el->add_child("FramesPerSecond")->add_child_text (raw_convert<string> (_frames_per_second));
	el->add_child("J2KBandwidth")->add_child_text (raw_convert<string> (_j2k_bandwidth));
	el->add_child("Resolution")->add_child_text (raw_convert<string> (int (_resolution)));
	_frame->add_metadata (el);
}

Eyes
DCPVideoFrame::eyes () const
{
	return _frame->eyes ();
}

EncodedData::EncodedData (int s)
	: _data (new uint8_t[s])
	, _size (s)
{

}

EncodedData::EncodedData (boost::filesystem::path file)
{
	_size = boost::filesystem::file_size (file);
	_data = new uint8_t[_size];

	FILE* f = fopen_boost (file, "rb");
	if (!f) {
		throw FileError (_("could not open file for reading"), file);
	}

	size_t const r = fread (_data, 1, _size, f);
	if (r != size_t (_size)) {
		fclose (f);
		throw FileError (_("could not read encoded data"), file);
	}

	fclose (f);
}


EncodedData::~EncodedData ()
{
	delete[] _data;
}

/** Write this data to a J2K file.
 *  @param Film Film.
 *  @param frame DCP frame index.
 */
void
EncodedData::write (shared_ptr<const Film> film, int frame, Eyes eyes) const
{
	boost::filesystem::path const tmp_j2c = film->j2c_path (frame, eyes, true);

	FILE* f = fopen_boost (tmp_j2c, "wb");

	if (!f) {
		throw WriteFileError (tmp_j2c, errno);
	}

	fwrite (_data, 1, _size, f);
	fclose (f);

	boost::filesystem::path const real_j2c = film->j2c_path (frame, eyes, false);

	/* Rename the file from foo.j2c.tmp to foo.j2c now that it is complete */
	boost::filesystem::rename (tmp_j2c, real_j2c);
}

void
EncodedData::write_info (shared_ptr<const Film> film, int frame, Eyes eyes, libdcp::FrameInfo info) const
{
	FILE* file = 0;
	if (boost::filesystem::exists (film->info_file ())) {
		file = fopen_boost (film->info_file(), "r+b");
	} else {
		file = fopen_boost (film->info_file(), "wb");
	}
	if (!file) {
		throw OpenFileError (film->info_file ());
	}
	write_frame_info (file, frame, eyes, info);
	fclose (file);
}

/** Send this data to a socket.
 *  @param socket Socket
 */
void
EncodedData::send (shared_ptr<Socket> socket)
{
	socket->write (_size);
	socket->write (_data, _size);
}

LocallyEncodedData::LocallyEncodedData (uint8_t* d, int s)
	: EncodedData (s)
{
	memcpy (_data, d, s);
}

/** @param s Size of data in bytes */
RemotelyEncodedData::RemotelyEncodedData (int s)
	: EncodedData (s)
{

}
