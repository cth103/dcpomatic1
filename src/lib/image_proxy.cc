/*
    Copyright (C) 2014-2015 Carl Hetherington <cth@carlh.net>

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

#include <Magick++.h>
#include <libdcp/util.h>
#include "raw_convert.h"
#include "image_proxy.h"
#include "image.h"
#include "exceptions.h"
#include "cross.h"
#include "log.h"

#include "i18n.h"

#define LOG_TIMING(...) _log->microsecond_log (String::compose (__VA_ARGS__), Log::TYPE_TIMING);

using std::cout;
using std::string;
using boost::shared_ptr;

ImageProxy::ImageProxy (shared_ptr<Log> log)
	: _log (log)
{

}

RawImageProxy::RawImageProxy (shared_ptr<Image> image, shared_ptr<Log> log)
	: ImageProxy (log)
	, _image (image)
{

}

RawImageProxy::RawImageProxy (shared_ptr<cxml::Node> xml, shared_ptr<Socket> socket, shared_ptr<Log> log)
	: ImageProxy (log)
{
	libdcp::Size size (
		xml->number_child<int> ("Width"), xml->number_child<int> ("Height")
		);

	_image.reset (new Image (static_cast<AVPixelFormat> (xml->number_child<int> ("PixelFormat")), size, true));
	_image->read_from_socket (socket);
}

shared_ptr<Image>
RawImageProxy::image () const
{
	return _image;
}

void
RawImageProxy::add_metadata (xmlpp::Node* node) const
{
	node->add_child("Type")->add_child_text (N_("Raw"));
	node->add_child("Width")->add_child_text (raw_convert<string> (_image->size().width));
	node->add_child("Height")->add_child_text (raw_convert<string> (_image->size().height));
	node->add_child("PixelFormat")->add_child_text (raw_convert<string> (_image->pixel_format ()));
}

void
RawImageProxy::send_binary (shared_ptr<Socket> socket) const
{
	_image->write_to_socket (socket);
}

MagickImageProxy::MagickImageProxy (boost::filesystem::path path, shared_ptr<Log> log)
	: ImageProxy (log)
{
	/* Read the file into a Blob */

	boost::uintmax_t const size = boost::filesystem::file_size (path);
	FILE* f = fopen_boost (path, "rb");
	if (!f) {
		throw OpenFileError (path);
	}

	uint8_t* data = new uint8_t[size];
	if (fread (data, 1, size, f) != size) {
		delete[] data;
		throw ReadFileError (path);
	}

	fclose (f);
	_blob.update (data, size);
	delete[] data;
}

MagickImageProxy::MagickImageProxy (shared_ptr<cxml::Node>, shared_ptr<Socket> socket, shared_ptr<Log> log)
	: ImageProxy (log)
{
	uint32_t const size = socket->read_uint32 ();
	uint8_t* data = new uint8_t[size];
	socket->read (data, size);
	_blob.update (data, size);
	delete[] data;
}

shared_ptr<Image>
MagickImageProxy::image () const
{
	boost::mutex::scoped_lock lm (_mutex);

	if (_image) {
		return _image;
	}

	LOG_TIMING ("[%1] MagickImageProxy begins decode and convert of %2 bytes", boost::this_thread::get_id(), _blob.length());

	Magick::Image* magick_image = 0;
	string error;
	try {
		magick_image = new Magick::Image (_blob);
	} catch (Magick::Exception& e) {
		error = e.what ();
	}

	if (!magick_image) {
		/* ImageMagick cannot auto-detect Targa files, it seems, so try here with an
		   explicit format.  I can't find it documented that passing a (0, 0) geometry
		   is allowed, but it seems to work.
		*/
		try {
			magick_image = new Magick::Image (_blob, Magick::Geometry (0, 0), "TGA");
		} catch (...) {

		}
	}

	if (!magick_image) {
		/* If we failed both an auto-detect and a forced-Targa we give the error from
		   the auto-detect.
		*/
		throw DecodeError (String::compose (_("Could not decode image file (%1)"), error));
	}

	LOG_TIMING ("[%1] MagickImageProxy decode finished", boost::this_thread::get_id ());

	libdcp::Size size (magick_image->columns(), magick_image->rows());

	_image.reset (new Image (PIX_FMT_RGB24, size, true));

	/* Write line-by-line here as _image must be aligned, and write() cannot be told about strides */
	uint8_t* p = _image->data()[0];
	for (int i = 0; i < size.height; ++i) {
#ifdef DCPOMATIC_IMAGE_MAGICK
		using namespace MagickCore;
#else
		using namespace MagickLib;
#endif
		magick_image->write (0, i, size.width, 1, "RGB", CharPixel, p);
		p += _image->stride()[0];
	}

	delete magick_image;

	LOG_TIMING ("[%1] MagickImageProxy completes decode and convert of %2 bytes", boost::this_thread::get_id(), _blob.length());

	return _image;
}

void
MagickImageProxy::add_metadata (xmlpp::Node* node) const
{
	node->add_child("Type")->add_child_text (N_("Magick"));
}

void
MagickImageProxy::send_binary (shared_ptr<Socket> socket) const
{
	socket->write (_blob.length ());
	socket->write ((uint8_t *) _blob.data (), _blob.length ());
}

shared_ptr<ImageProxy>
image_proxy_factory (shared_ptr<cxml::Node> xml, shared_ptr<Socket> socket, shared_ptr<Log> log)
{
	if (xml->string_child("Type") == N_("Raw")) {
		return shared_ptr<ImageProxy> (new RawImageProxy (xml, socket, log));
	} else if (xml->string_child("Type") == N_("Magick")) {
		return shared_ptr<MagickImageProxy> (new MagickImageProxy (xml, socket, log));
	}

	throw NetworkError (_("Unexpected image type received by server"));
}
