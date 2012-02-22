/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>
    Copyright (C) 2000-2007 Paul Davis

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

#include <sstream>
#include <iomanip>
#include <iostream>
#include <execinfo.h>
#include <cxxabi.h>
#include <sys/time.h>
#include <boost/algorithm/string.hpp>
#include <openjpeg.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libpostproc/postprocess.h>
#include <libavutil/pixfmt.h>
}
#include "util.h"
#include "exceptions.h"
#include "scaler.h"

#ifdef DEBUG_HASH
#include <mhash.h>
#endif

using namespace std;
using namespace boost;

/** Convert some number of seconds to a string representation
 *  in hours, minutes and seconds.
 *
 *  @param s Seconds.
 *  @return String of the form H:M:S (where H is hours, M
 *  is minutes and S is seconds).
 */
 
string
seconds_to_hms (int s)
{
	int m = s / 60;
	s -= (m * 60);
	int h = m / 60;
	m -= (h * 60);

	stringstream hms;
	hms << h << ":";
	hms.width (2);
	hms << setfill ('0') << m << ":";
	hms.width (2);
	hms << setfill ('0') << s;

	return hms.str ();
}

string
seconds_to_approximate_hms (int s)
{
	int m = s / 60;
	s -= (m * 60);
	int h = m / 60;
	m -= (h * 60);

	stringstream ap;
	
	if (h > 0) {
		if (m > 30) {
			ap << (h + 1) << " hours";
		} else {
			if (h == 1) {
				ap << "1 hour";
			} else {
				ap << h << " hours";
			}
		}
	} else if (m > 0) {
		if (m == 1) {
			ap << "1 minute";
		} else {
			ap << m << " minutes";
		}
	} else {
		ap << s << " seconds";
	}

	return ap.str ();
}

Gtk::Label &
left_aligned_label (string t)
{
	Gtk::Label* l = Gtk::manage (new Gtk::Label (t));
	l->set_alignment (0, 0.5);
	return *l;
}

static string
demangle (string l)
{
	string::size_type const b = l.find_first_of ("(");
	if (b == string::npos) {
		return l;
	}

	string::size_type const p = l.find_last_of ("+");
	if (p == string::npos) {
		return l;
	}

	if ((p - b) <= 1) {
		return l;
	}
	
	string const fn = l.substr (b + 1, p - b - 1);

	int status;
	try {
		
		char* realname = abi::__cxa_demangle (fn.c_str(), 0, 0, &status);
		string d (realname);
		free (realname);
		return d;
		
	} catch (std::exception) {
		
	}
	
	return l;
}

void
stacktrace (ostream& out, int levels)
{
	void *array[200];
	size_t size;
	char **strings;
	size_t i;
     
	size = backtrace (array, 200);
	strings = backtrace_symbols (array, size);
     
	if (strings) {
		for (i = 0; i < size && (levels == 0 || i < size_t(levels)); i++) {
			out << "  " << demangle (strings[i]) << endl;
		}
		
		free (strings);
	}
}

string
audio_sample_format_to_string (AVSampleFormat s)
{
	/* Our sample format handling is not exactly complete */
	
	switch (s) {
	case AV_SAMPLE_FMT_S16:
		return "S16";
	default:
		break;
	}

	return "Unknown";
}

AVSampleFormat
audio_sample_format_from_string (string s)
{
	if (s == "S16") {
		return AV_SAMPLE_FMT_S16;
	}

	return AV_SAMPLE_FMT_NONE;
}

static string
opendcp_version ()
{
	FILE* f = popen ("opendcp_xml", "r");
	if (f == 0) {
		throw EncodeError ("could not run opendcp_xml to check version");
	}

	string version = "unknown";
	
	while (!feof (f)) {
		char* buf = 0;
		size_t n = 0;
		getline (&buf, &n, f);
		if (n > 0) {
			string s (buf);
			vector<string> b;
			split (b, s, is_any_of (" "));
			if (b.size() >= 3 && b[0] == "OpenDCP" && b[1] == "version") {
				version = b[2];
			}
		}
	}

	pclose (f);

	return version;
}

static string
ffmpeg_version_to_string (int v)
{
	stringstream s;
	s << ((v & 0xff0000) >> 16) << "." << ((v & 0xff00) >> 8) << "." << (v & 0xff);
	return s.str ();
}

string
dependency_version_summary ()
{
	stringstream s;
	s << "libopenjpeg " << opj_version () << ", "
	  << "opendcp " << opendcp_version () << ", "
	  << "libswresample " << ffmpeg_version_to_string (swresample_version()) << ", "
	  << "libavcodec " << ffmpeg_version_to_string (avcodec_version()) << ", "
	  << "libavfilter " << ffmpeg_version_to_string (avfilter_version()) << ", "
	  << "libavformat " << ffmpeg_version_to_string (avformat_version()) << ", "
	  << "libavutil " << ffmpeg_version_to_string (avutil_version()) << ", "
	  << "libpostproc " << ffmpeg_version_to_string (postproc_version()) << ", "
	  << "libswscale " << ffmpeg_version_to_string (swscale_version());

	return s.str ();
}

void
fd_write (int fd, uint8_t const * data, int size)
{
	uint8_t const * p = data;
	while (size) {
		int const n = write (fd, p, size);
		if (n < 0) {
			stringstream s;
			s << "could not write (" << strerror (errno) << ")";
			throw NetworkError (s.str ());
		}

		size -= n;
		p += n;
	}
}

double
seconds (struct timeval t)
{
	return t.tv_sec + (double (t.tv_usec) / 1e6);
}

SocketReader::SocketReader (int fd)
	: _fd (fd)
	, _buffer_data (0)
{

}

void
SocketReader::consume (int size)
{
	_buffer_data -= size;
	if (_buffer_data > 0) {
		memmove (_buffer, _buffer + size, _buffer_data);
	}
}

void
SocketReader::read_definite_and_consume (uint8_t* data, int size)
{
	int const from_buffer = min (_buffer_data, size);
	if (from_buffer > 0) {
		/* Get data from our buffer */
		memcpy (data, _buffer, from_buffer);
		consume (from_buffer);
		/* Update our output state */
		data += from_buffer;
		size -= from_buffer;
	}

	/* read() the rest */
	while (size > 0) {
		int const n = ::read (_fd, data, size);
		if (n < 0) {
			throw NetworkError ("could not read");
		}

		data += n;
		size -= n;
	}
}

void
SocketReader::read_indefinite (uint8_t* data, int size)
{
	assert (size < int (sizeof (_buffer)));

	/* Amount of extra data we need to read () */
	int to_read = size - _buffer_data;
	while (to_read > 0) {
		/* read as much of it as we can (into our buffer) */
		int const n = ::read (_fd, _buffer + _buffer_data, to_read);
		if (n < 0) {
			throw NetworkError ("could not read");
		}

		to_read -= n;
		_buffer_data += n;
	}

	assert (_buffer_data >= size);

	/* copy data into the output buffer */
	assert (size >= _buffer_data);
	memcpy (data, _buffer, size);
}

int
Image::lines (int n) const
{
	switch (_pixel_format) {
	case PIX_FMT_YUV420P:
		if (n == 0) {
			return size().height;
		} else {
			return size().height / 2;
		}
		break;
	default:
		assert (false);
	}

	return 0;
}

int
Image::components () const
{
	switch (_pixel_format) {
	case PIX_FMT_YUV420P:
		return 3;
	default:
		assert (false);
	}

	return 0;
}

#ifdef DEBUG_HASH
void
Image::hash () const
{
	MHASH ht = mhash_init (MHASH_MD5);
	if (ht == MHASH_FAILED) {
		throw EncodeError ("could not create hash thread");
	}
	
	for (int i = 0; i < components(); ++i) {
		mhash (ht, data()[i], line_size()[i] * lines(i));
	}
	
	uint8_t hash[16];
	mhash_deinit (ht, hash);
	
	printf ("YUV input: ");
	for (int i = 0; i < int (mhash_get_block_size (MHASH_MD5)); ++i) {
		printf ("%.2x", hash[i]);
	}
	printf ("\n");
}
#endif

/** Scale this image to a given size and convert it to RGB.
 *  Caller must pass both returned values to av_free ().
 *  @param out_size Output image size in pixels.
 *  @param scaler Scaler to use.
 */

pair<AVFrame *, uint8_t *>
Image::scale_and_convert_to_rgb (Size out_size, Scaler const * scaler) const
{
	assert (scaler);
	
	AVFrame* frame_out = avcodec_alloc_frame ();
	if (frame_out == 0) {
		throw EncodeError ("could not allocate frame");
	}

	struct SwsContext* scale_context = sws_getContext (
		size().width, size().height, pixel_format(),
		out_size.width, out_size.height, PIX_FMT_RGB24,
		scaler->ffmpeg_id (), 0, 0, 0
		);

	uint8_t* rgb = (uint8_t *) av_malloc (out_size.width * out_size.height * 3);
	avpicture_fill ((AVPicture *) frame_out, rgb, PIX_FMT_RGB24, out_size.width, out_size.height);
	
	/* Scale and convert from YUV to RGB */
	sws_scale (
		scale_context,
		data(), line_size(),
		0, size().height,
		frame_out->data, frame_out->linesize
		);

	sws_freeContext (scale_context);

	return make_pair (frame_out, rgb);
}

FilterBuffer::FilterBuffer (PixelFormat p, AVFilterBufferRef* b)
	: Image (p)
	, _buffer (b)
{

}

FilterBuffer::~FilterBuffer ()
{
	avfilter_unref_buffer (_buffer);
}

uint8_t **
FilterBuffer::data () const
{
	return _buffer->data;
}

int *
FilterBuffer::line_size () const
{
	return _buffer->linesize;
}

Size
FilterBuffer::size () const
{
	return Size (_buffer->video->w, _buffer->video->h);
}

AllocImage::AllocImage (PixelFormat p, Size s)
	: Image (p)
	, _size (s)
{
	_data = (uint8_t **) av_malloc (components() * sizeof (uint8_t *));
	_line_size = (int *) av_malloc (components() * sizeof (int));
	
	for (int i = 0; i < components(); ++i) {
		_data[i] = 0;
		_line_size[i] = 0;
	}
}

AllocImage::~AllocImage ()
{
	for (int i = 0; i < components(); ++i) {
		av_free (_data[i]);
	}

	av_free (_data);
	av_free (_line_size);
}

void
AllocImage::set_line_size (int i, int s)
{
	_line_size[i] = s;
	_data[i] = (uint8_t *) av_malloc (s * lines (i));
}

uint8_t **
AllocImage::data () const
{
	return _data;
}

int *
AllocImage::line_size () const
{
	return _line_size;
}

Size
AllocImage::size () const
{
	return _size;
}

PeriodTimer::PeriodTimer (string n)
	: _name (n)
{
	gettimeofday (&_start, 0);
}

PeriodTimer::~PeriodTimer ()
{
	struct timeval stop;
	gettimeofday (&stop, 0);
	cout << "T: " << _name << ": " << (seconds (stop) - seconds (_start)) << "\n";
}

	
StateTimer::StateTimer (string n, string s)
	: _name (n)
{
	struct timeval t;
	gettimeofday (&t, 0);
	_time = seconds (t);
	_state = s;
}

void
StateTimer::set_state (string s)
{
	double const last = _time;
	struct timeval t;
	gettimeofday (&t, 0);
	_time = seconds (t);

	if (_totals.find (s) == _totals.end ()) {
		_totals[s] = 0;
	}

	_totals[_state] += _time - last;
	_state = s;
}

StateTimer::~StateTimer ()
{
	if (_state.empty ()) {
		return;
	}

	
	set_state ("");

	cout << _name << ":\n";
	for (map<string, double>::iterator i = _totals.begin(); i != _totals.end(); ++i) {
		cout << "\t" << i->first << " " << i->second << "\n";
	}
}
