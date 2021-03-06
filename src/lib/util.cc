/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>
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

/** @file src/lib/util.cc
 *  @brief Some utility functions and classes.
 */

#include <iomanip>
#include <iostream>
#include <fstream>
#include <climits>
#include <stdexcept>
#ifdef DCPOMATIC_POSIX
#include <execinfo.h>
#include <cxxabi.h>
#endif
#include <libssh/libssh.h>
#include <signal.h>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#ifdef DCPOMATIC_WINDOWS
#include <boost/locale.hpp>
#include <dbghelp.h>
#endif
#include <glib.h>
#include <openjpeg.h>
#ifdef DCPOMATIC_IMAGE_MAGICK
#include <magick/MagickCore.h>
#else
#include <magick/common.h>
#include <magick/magick_config.h>
#endif
#include <magick/version.h>
#include <libdcp/version.h>
#include <libdcp/util.h>
#include <libdcp/signer_chain.h>
#include <libdcp/signer.h>
#include <libdcp/asset.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfiltergraph.h>
#include <libavutil/pixfmt.h>
}
#include "raw_convert.h"
#include "util.h"
#include "exceptions.h"
#include "scaler.h"
#include "dcp_content_type.h"
#include "filter.h"
#include "sound_processor.h"
#include "config.h"
#include "ratio.h"
#include "job.h"
#include "cross.h"
#include "video_content.h"
#include "md5_digester.h"
#include "safe_stringstream.h"
#include "colour_conversion.h"

#include "i18n.h"

using std::string;
using std::setfill;
using std::ostream;
using std::endl;
using std::vector;
using std::hex;
using std::setw;
using std::ios;
using std::min;
using std::max;
using std::list;
using std::multimap;
using std::map;
using std::istream;
using std::numeric_limits;
using std::pair;
using std::cout;
using std::bad_alloc;
using std::streampos;
using std::set_terminate;
using boost::shared_ptr;
using boost::thread;
using boost::optional;
using libdcp::Size;

/** Path to our executable, required by the stacktrace stuff and filled
 *  in during App::onInit().
 */
std::string program_name;
static boost::thread::id ui_thread;
static boost::filesystem::path backtrace_file;

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

	SafeStringStream hms;
	hms << h << N_(":");
	hms.width (2);
	hms << std::setfill ('0') << m << N_(":");
	hms.width (2);
	hms << std::setfill ('0') << s;

	return hms.str ();
}

/** @param s Number of seconds.
 *  @return String containing an approximate description of s (e.g. "about 2 hours")
 */
string
seconds_to_approximate_hms (int s)
{
	int m = s / 60;
	s -= (m * 60);
	int h = m / 60;
	m -= (h * 60);

	SafeStringStream ap;

	bool const hours = h > 0;
	bool const minutes = h < 10 && m > 0;
	bool const seconds = m < 10 && s > 0;

	if (hours) {
		if (m > 30 && !minutes) {
			ap << (h + 1) << N_(" ") << _("hours");
		} else {
			ap << h << N_(" ");
			if (h == 1) {
				ap << _("hour");
			} else {
				ap << _("hours");
			}
		}

		if (minutes | seconds) {
			ap << N_(" ");
		}
	}

	if (minutes) {
		/* Minutes */
		if (s > 30 && !seconds) {
			ap << (m + 1) << N_(" ") << _("minutes");
		} else {
			ap << m << N_(" ");
			if (m == 1) {
				ap << _("minute");
			} else {
				ap << _("minutes");
			}
		}

		if (seconds) {
			ap << N_(" ");
		}
	}

	if (seconds) {
		/* Seconds */
		ap << s << N_(" ");
		if (s == 1) {
			ap << _("second");
		} else {
			ap << _("seconds");
		}
	}

	return ap.str ();
}

#ifdef DCPOMATIC_POSIX
/** @param l Mangled C++ identifier.
 *  @return Demangled version.
 */
static string
demangle (string l)
{
	string::size_type const b = l.find_first_of (N_("("));
	if (b == string::npos) {
		return l;
	}

	string::size_type const p = l.find_last_of (N_("+"));
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

/** Write a stacktrace to an ostream.
 *  @param out Stream to write to.
 *  @param levels Number of levels to go up the call stack.
 */
void
stacktrace (ostream& out, int levels)
{
	void *array[200];
	size_t size = backtrace (array, 200);
	char** strings = backtrace_symbols (array, size);

	if (strings) {
		for (size_t i = 0; i < size && (levels == 0 || i < size_t(levels)); i++) {
			out << N_("  ") << demangle (strings[i]) << "\n";
		}

		free (strings);
	}
}
#endif

/** @param v Version as used by FFmpeg.
 *  @return A string representation of v.
 */
static string
ffmpeg_version_to_string (int v)
{
	SafeStringStream s;
	s << ((v & 0xff0000) >> 16) << N_(".") << ((v & 0xff00) >> 8) << N_(".") << (v & 0xff);
	return s.str ();
}

/** Return a user-readable string summarising the versions of our dependencies */
string
dependency_version_summary ()
{
	SafeStringStream s;
	s << N_("libopenjpeg ") << opj_version () << N_(", ")
	  << N_("libavcodec ") << ffmpeg_version_to_string (avcodec_version()) << N_(", ")
	  << N_("libavfilter ") << ffmpeg_version_to_string (avfilter_version()) << N_(", ")
	  << N_("libavformat ") << ffmpeg_version_to_string (avformat_version()) << N_(", ")
	  << N_("libavutil ") << ffmpeg_version_to_string (avutil_version()) << N_(", ")
	  << N_("libswscale ") << ffmpeg_version_to_string (swscale_version()) << N_(", ")
	  << MagickVersion << N_(", ")
	  << N_("libssh ") << ssh_version (0) << N_(", ")
	  << N_("libdcp ") << libdcp::version << N_(" git ") << libdcp::git_commit;

	return s.str ();
}

double
seconds (struct timeval t)
{
	return t.tv_sec + (double (t.tv_usec) / 1e6);
}

#ifdef DCPOMATIC_WINDOWS
/** Resolve symbol name and source location given the path to the executable */
int
addr2line (void const * const addr)
{
	char addr2line_cmd[512] = {0};
	sprintf(addr2line_cmd, "addr2line -f -p -e %.256s %p > %s", program_name.c_str(), addr, backtrace_file.string().c_str());
	return system(addr2line_cmd);
}

/** This is called when C signals occur on Windows (e.g. SIGSEGV)
 *  (NOT C++ exceptions!).  We write a backtrace to backtrace_file by dark means.
 *  Adapted from code here: http://spin.atomicobject.com/2013/01/13/exceptions-stack-traces-c/
 */
LONG WINAPI
exception_handler (struct _EXCEPTION_POINTERS * info)
{
 	FILE* f = fopen_boost (backtrace_file, "w");
	fprintf (f, "C-style exception %d\n", info->ExceptionRecord->ExceptionCode);
	fclose(f);

	if (info->ExceptionRecord->ExceptionCode != EXCEPTION_STACK_OVERFLOW) {
		CONTEXT* context = info->ContextRecord;
		SymInitialize (GetCurrentProcess (), 0, true);

		STACKFRAME frame = { 0 };

		/* setup initial stack frame */
#if _WIN64
		frame.AddrPC.Offset    = context->Rip;
		frame.AddrStack.Offset = context->Rsp;
		frame.AddrFrame.Offset = context->Rbp;
#else
		frame.AddrPC.Offset    = context->Eip;
		frame.AddrStack.Offset = context->Esp;
		frame.AddrFrame.Offset = context->Ebp;
#endif
		frame.AddrPC.Mode      = AddrModeFlat;
		frame.AddrStack.Mode   = AddrModeFlat;
		frame.AddrFrame.Mode   = AddrModeFlat;

		while (
			StackWalk (
				IMAGE_FILE_MACHINE_I386,
				GetCurrentProcess (),
				GetCurrentThread (),
				&frame,
				context,
				0,
				SymFunctionTableAccess,
				SymGetModuleBase,
				0
				)
			) {
			addr2line((void *) frame.AddrPC.Offset);
		}
	} else {
#ifdef _WIN64
		addr2line ((void *) info->ContextRecord->Rip);
#else
		addr2line ((void *) info->ContextRecord->Eip);
#endif
 	}

 	return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void
set_backtrace_file (boost::filesystem::path p)
{
	backtrace_file = p;
}

/** This is called when there is an unhandled exception.  Any
 *  backtrace in this function is useless on Windows as the stack has
 *  already been unwound from the throw; we have the gdb wrap hack to
 *  cope with that.
 */
void
terminate ()
{
	static bool tried_throw = false;

	try {
		// try once to re-throw currently active exception
		if (!tried_throw) {
			tried_throw = true;
			throw;
		}
	}
	catch (const std::exception &e) {
		std::cerr << __FUNCTION__ << " caught unhandled exception. what(): "
			  << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << __FUNCTION__ << " caught unknown/unhandled exception."
			  << std::endl;
	}

#ifdef DCPOMATIC_POSIX
	stacktrace (cout, 50);
#endif
	abort();
}

/** Call the required functions to set up DCP-o-matic's static arrays, etc.
 *  Must be called from the UI thread, if there is one.
 */
void
dcpomatic_setup ()
{
#ifdef DCPOMATIC_WINDOWS
	boost::filesystem::path p = g_get_user_config_dir ();
	p /= "backtrace.txt";
	set_backtrace_file (p);
	SetUnhandledExceptionFilter (exception_handler);

	/* Dark voodoo which, I think, gets boost::filesystem::path to
	   correctly convert UTF-8 strings to paths, and also paths
	   back to UTF-8 strings (on path::string()).

	   After this, constructing boost::filesystem::paths from strings
	   converts from UTF-8 to UTF-16 inside the path.  Then
	   path::string().c_str() gives UTF-8 and
	   path::c_str()          gives UTF-16.

	   This is all Windows-only.  AFAICT Linux/OS X use UTF-8 everywhere,
	   so things are much simpler.
	*/
	std::locale::global (boost::locale::generator().generate (""));
	boost::filesystem::path::imbue (std::locale ());
#endif

	avfilter_register_all ();

#ifdef DCPOMATIC_OSX
	/* Add our lib directory to the libltdl search path so that
	   xmlsec can find xmlsec1-openssl.
	*/
	boost::filesystem::path lib = app_contents ();
	lib /= "lib";
	setenv ("LTDL_LIBRARY_PATH", lib.c_str (), 1);
#endif

	set_terminate (terminate);

	libdcp::init ();

	Ratio::setup_ratios ();
	PresetColourConversion::setup_colour_conversion_presets ();
	VideoContentScale::setup_scales ();
	DCPContentType::setup_dcp_content_types ();
	Scaler::setup_scalers ();
	Filter::setup_filters ();
	SoundProcessor::setup_sound_processors ();

	ui_thread = boost::this_thread::get_id ();
}

#ifdef DCPOMATIC_WINDOWS
boost::filesystem::path
mo_path ()
{
	wchar_t buffer[512];
	GetModuleFileName (0, buffer, 512 * sizeof(wchar_t));
	boost::filesystem::path p (buffer);
	p = p.parent_path ();
	p = p.parent_path ();
	p /= "locale";
	return p;
}
#endif

#ifdef DCPOMATIC_OSX
boost::filesystem::path
mo_path ()
{
	return "DCP-o-matic.app/Contents/Resources";
}
#endif

void
dcpomatic_setup_gettext_i18n (string lang)
{
#ifdef DCPOMATIC_LINUX
	lang += ".UTF8";
#endif

	if (!lang.empty ()) {
		/* Override our environment language.  Note that the caller must not
		   free the string passed into putenv().
		*/
		string s = String::compose ("LANGUAGE=%1", lang);
		putenv (strdup (s.c_str ()));
		s = String::compose ("LANG=%1", lang);
		putenv (strdup (s.c_str ()));
		s = String::compose ("LC_ALL=%1", lang);
		putenv (strdup (s.c_str ()));
	}

	setlocale (LC_ALL, "");
	textdomain ("libdcpomatic");

#if defined(DCPOMATIC_WINDOWS) || defined(DCPOMATIC_OSX)
	bindtextdomain ("libdcpomatic", mo_path().string().c_str());
	bind_textdomain_codeset ("libdcpomatic", "UTF8");
#endif

#ifdef DCPOMATIC_LINUX
	bindtextdomain ("libdcpomatic", POSIX_LOCALE_PREFIX);
#endif
}

/** @param s A string.
 *  @return Parts of the string split at spaces, except when a space is within quotation marks.
 */
vector<string>
split_at_spaces_considering_quotes (string s)
{
	vector<string> out;
	bool in_quotes = false;
	string c;
	for (string::size_type i = 0; i < s.length(); ++i) {
		if (s[i] == ' ' && !in_quotes) {
			out.push_back (c);
			c = N_("");
		} else if (s[i] == '"') {
			in_quotes = !in_quotes;
		} else {
			c += s[i];
		}
	}

	out.push_back (c);
	return out;
}

/** Compute a digest of the first and last `size' bytes of a set of files. */
string
md5_digest_head_tail (vector<boost::filesystem::path> files, boost::uintmax_t size)
{
	boost::scoped_array<char> buffer (new char[size]);
	MD5Digester digester;

	/* Head */
	boost::uintmax_t to_do = size;
	char* p = buffer.get ();
	int i = 0;
	while (i < int64_t (files.size()) && to_do > 0) {
		FILE* f = fopen_boost (files[i], "rb");
		if (!f) {
			throw OpenFileError (files[i].string());
		}

		boost::uintmax_t this_time = min (to_do, boost::filesystem::file_size (files[i]));
		fread (p, 1, this_time, f);
		p += this_time;
 		to_do -= this_time;
		fclose (f);

		++i;
	}
	digester.add (buffer.get(), size - to_do);

	/* Tail */
	to_do = size;
	p = buffer.get ();
	i = files.size() - 1;
	while (i >= 0 && to_do > 0) {
		FILE* f = fopen_boost (files[i], "rb");
		if (!f) {
			throw OpenFileError (files[i].string());
		}

		boost::uintmax_t this_time = min (to_do, boost::filesystem::file_size (files[i]));
		dcpomatic_fseek (f, -this_time, SEEK_END);
		fread (p, 1, this_time, f);
		p += this_time;
		to_do -= this_time;
		fclose (f);

		--i;
	}
	digester.add (buffer.get(), size - to_do);

	return digester.get ();
}

Socket::Socket (int timeout)
	: _deadline (_io_service)
	, _socket (_io_service)
	, _acceptor (0)
	, _timeout (timeout)
{
	_deadline.expires_at (boost::posix_time::pos_infin);
	check ();
}

Socket::~Socket ()
{
	delete _acceptor;
}

void
Socket::check ()
{
	if (_deadline.expires_at() <= boost::asio::deadline_timer::traits_type::now ()) {
		if (_acceptor) {
			_acceptor->cancel ();
		} else {
			_socket.close ();
		}
		_deadline.expires_at (boost::posix_time::pos_infin);
	}

	_deadline.async_wait (boost::bind (&Socket::check, this));
}

/** Blocking connect.
 *  @param endpoint End-point to connect to.
 */
void
Socket::connect (boost::asio::ip::tcp::endpoint endpoint)
{
	_deadline.expires_from_now (boost::posix_time::seconds (_timeout));
	boost::system::error_code ec = boost::asio::error::would_block;
	_socket.async_connect (endpoint, boost::lambda::var(ec) = boost::lambda::_1);
	do {
		_io_service.run_one();
	} while (ec == boost::asio::error::would_block);

	if (ec) {
		throw NetworkError (String::compose (_("error during async_connect (%1)"), ec.value ()));
	}

	if (!_socket.is_open ()) {
		throw NetworkError (_("connect timed out"));
	}
}

void
Socket::accept (int port)
{
	_acceptor = new boost::asio::ip::tcp::acceptor (_io_service, boost::asio::ip::tcp::endpoint (boost::asio::ip::tcp::v4(), port));

	_deadline.expires_from_now (boost::posix_time::seconds (_timeout));
	boost::system::error_code ec = boost::asio::error::would_block;
	_acceptor->async_accept (_socket, boost::lambda::var(ec) = boost::lambda::_1);
	do {
		_io_service.run_one ();
	} while (ec == boost::asio::error::would_block);

	delete _acceptor;
	_acceptor = 0;

	if (ec) {
		throw NetworkError (String::compose (_("error during async_accept (%1)"), ec.value ()));
	}
}

/** Blocking write.
 *  @param data Buffer to write.
 *  @param size Number of bytes to write.
 */
void
Socket::write (uint8_t const * data, int size)
{
	_deadline.expires_from_now (boost::posix_time::seconds (_timeout));
	boost::system::error_code ec = boost::asio::error::would_block;

	boost::asio::async_write (_socket, boost::asio::buffer (data, size), boost::lambda::var(ec) = boost::lambda::_1);

	do {
		_io_service.run_one ();
	} while (ec == boost::asio::error::would_block);

	if (ec) {
		throw NetworkError (String::compose (_("error during async_write (%1)"), ec.value ()));
	}
}

void
Socket::write (uint32_t v)
{
	v = htonl (v);
	write (reinterpret_cast<uint8_t*> (&v), 4);
}

/** Blocking read.
 *  @param data Buffer to read to.
 *  @param size Number of bytes to read.
 */
void
Socket::read (uint8_t* data, int size)
{
	_deadline.expires_from_now (boost::posix_time::seconds (_timeout));
	boost::system::error_code ec = boost::asio::error::would_block;

	boost::asio::async_read (_socket, boost::asio::buffer (data, size), boost::lambda::var(ec) = boost::lambda::_1);

	do {
		_io_service.run_one ();
	} while (ec == boost::asio::error::would_block);

	if (ec) {
		throw NetworkError (String::compose (_("error during async_read (%1)"), ec.value ()));
	}
}

uint32_t
Socket::read_uint32 ()
{
	uint32_t v;
	read (reinterpret_cast<uint8_t *> (&v), 4);
	return ntohl (v);
}

/** Round a number up to the nearest multiple of another number.
 *  @param c Index.
 *  @param s Array of numbers to round, indexed by c.
 *  @param t Multiple to round to.
 *  @return Rounded number.
 */
int
stride_round_up (int c, int const * stride, int t)
{
	int const a = stride[c] + (t - 1);
	return a - (a % t);
}

/** Read a sequence of key / value pairs from a text stream;
 *  the keys are the first words on the line, and the values are
 *  the remainder of the line following the key.  Lines beginning
 *  with # are ignored.
 *  @param s Stream to read.
 *  @return key/value pairs.
 */
multimap<string, string>
read_key_value (istream &s)
{
	multimap<string, string> kv;

	string line;
	while (getline (s, line)) {
		if (line.empty ()) {
			continue;
		}

		if (line[0] == '#') {
			continue;
		}

		if (line[line.size() - 1] == '\r') {
			line = line.substr (0, line.size() - 1);
		}

		size_t const s = line.find (' ');
		if (s == string::npos) {
			continue;
		}

		kv.insert (make_pair (line.substr (0, s), line.substr (s + 1)));
	}

	return kv;
}

string
get_required_string (multimap<string, string> const & kv, string k)
{
	if (kv.count (k) > 1) {
		throw StringError (N_("unexpected multiple keys in key-value set"));
	}

	multimap<string, string>::const_iterator i = kv.find (k);

	if (i == kv.end ()) {
		throw StringError (String::compose (_("missing key %1 in key-value set"), k));
	}

	return i->second;
}

int
get_required_int (multimap<string, string> const & kv, string k)
{
	string const v = get_required_string (kv, k);
	return raw_convert<int> (v);
}

float
get_required_float (multimap<string, string> const & kv, string k)
{
	string const v = get_required_string (kv, k);
	return raw_convert<float> (v);
}

string
get_optional_string (multimap<string, string> const & kv, string k)
{
	if (kv.count (k) > 1) {
		throw StringError (N_("unexpected multiple keys in key-value set"));
	}

	multimap<string, string>::const_iterator i = kv.find (k);
	if (i == kv.end ()) {
		return N_("");
	}

	return i->second;
}

int
get_optional_int (multimap<string, string> const & kv, string k)
{
	if (kv.count (k) > 1) {
		throw StringError (N_("unexpected multiple keys in key-value set"));
	}

	multimap<string, string>::const_iterator i = kv.find (k);
	if (i == kv.end ()) {
		return 0;
	}

	return raw_convert<int> (i->second);
}

/** Trip an assert if the caller is not in the UI thread */
void
ensure_ui_thread ()
{
	DCPOMATIC_ASSERT (boost::this_thread::get_id() == ui_thread);
}

/** @param v Content video frame.
 *  @param audio_sample_rate Source audio sample rate.
 *  @param frames_per_second Number of video frames per second.
 *  @return Equivalent number of audio frames for `v'.
 */
int64_t
video_frames_to_audio_frames (VideoContent::Frame v, float audio_sample_rate, float frames_per_second)
{
	return ((int64_t) v * audio_sample_rate / frames_per_second);
}

string
audio_channel_name (int c)
{
	DCPOMATIC_ASSERT (MAX_DCP_AUDIO_CHANNELS == 12);

	/// TRANSLATORS: these are the names of audio channels; Lfe (sub) is the low-frequency
	/// enhancement channel (sub-woofer).  HI is the hearing-impaired audio track and
	/// VI is the visually-impaired audio track (audio describe).
	string const channels[] = {
		_("Left"),
		_("Right"),
		_("Centre"),
		_("Lfe (sub)"),
		_("Left surround"),
		_("Right surround"),
		_("Hearing impaired"),
		_("Visually impaired"),
		_("Left centre"),
		_("Right centre"),
		_("Left rear surround"),
		_("Right rear surround"),
	};

	return channels[c];
}

bool
valid_image_file (boost::filesystem::path f)
{
	/* Ignore any files that start with ._ as they are probably OS X resource files */
	if (boost::starts_with (f.leaf().string(), "._")) {
		return false;
	}

	string ext = f.extension().string();
	transform (ext.begin(), ext.end(), ext.begin(), ::tolower);
	return (ext == ".tif" || ext == ".tiff" || ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || ext == ".tga" || ext == ".dpx");
}

string
tidy_for_filename (string f)
{
	string t;
	for (size_t i = 0; i < f.length(); ++i) {
		if (isalnum (f[i]) || f[i] == '_' || f[i] == '-') {
			t += f[i];
		} else {
			t += '_';
		}
	}

	return t;
}

shared_ptr<const libdcp::Signer>
make_signer ()
{
	boost::filesystem::path const sd = Config::instance()->signer_chain_directory ();

	/* Remake the chain if any of it is missing */

	list<boost::filesystem::path> files;
	files.push_back ("ca.self-signed.pem");
	files.push_back ("intermediate.signed.pem");
	files.push_back ("leaf.signed.pem");
	files.push_back ("leaf.key");

	list<boost::filesystem::path>::const_iterator i = files.begin();
	while (i != files.end()) {
		boost::filesystem::path p (sd);
		p /= *i;
		if (!boost::filesystem::exists (p)) {
			boost::filesystem::remove_all (sd);
			boost::filesystem::create_directories (sd);
			libdcp::make_signer_chain (sd, openssl_path ());
			break;
		}

		++i;
	}

	libdcp::CertificateChain chain;

	{
		boost::filesystem::path p (sd);
		p /= "ca.self-signed.pem";
		chain.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate (p)));
	}

	{
		boost::filesystem::path p (sd);
		p /= "intermediate.signed.pem";
		chain.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate (p)));
	}

	{
		boost::filesystem::path p (sd);
		p /= "leaf.signed.pem";
		chain.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate (p)));
	}

	boost::filesystem::path signer_key (sd);
	signer_key /= "leaf.key";

	return shared_ptr<const libdcp::Signer> (new libdcp::Signer (chain, signer_key));
}

map<string, string>
split_get_request (string url)
{
	enum {
		AWAITING_QUESTION_MARK,
		KEY,
		VALUE
	} state = AWAITING_QUESTION_MARK;

	map<string, string> r;
	string k;
	string v;
	for (size_t i = 0; i < url.length(); ++i) {
		switch (state) {
		case AWAITING_QUESTION_MARK:
			if (url[i] == '?') {
				state = KEY;
			}
			break;
		case KEY:
			if (url[i] == '=') {
				v.clear ();
				state = VALUE;
			} else {
				k += url[i];
			}
			break;
		case VALUE:
			if (url[i] == '&') {
				r.insert (make_pair (k, v));
				k.clear ();
				state = KEY;
			} else {
				v += url[i];
			}
			break;
		}
	}

	if (state == VALUE) {
		r.insert (make_pair (k, v));
	}

	return r;
}

libdcp::Size
fit_ratio_within (float ratio, libdcp::Size full_frame)
{
	if (ratio < full_frame.ratio ()) {
		return libdcp::Size (rint (full_frame.height * ratio), full_frame.height);
	}

	return libdcp::Size (full_frame.width, rint (full_frame.width / ratio));
}

void *
wrapped_av_malloc (size_t s)
{
	void* p = av_malloc (s);
	if (!p) {
		throw bad_alloc ();
	}
	return p;
}

string
entities_to_text (string e)
{
	boost::algorithm::replace_all (e, "%3A", ":");
	boost::algorithm::replace_all (e, "%2F", "/");
	return e;
}

int64_t
divide_with_round (int64_t a, int64_t b)
{
	if (a % b >= (b / 2)) {
		return (a + b - 1) / b;
	} else {
		return a / b;
	}
}

long
frame_info_position (int frame, Eyes eyes)
{
	static int const info_size = 48;

	switch (eyes) {
	case EYES_BOTH:
		return frame * info_size;
	case EYES_LEFT:
		return frame * info_size * 2;
	case EYES_RIGHT:
		return frame * info_size * 2 + info_size;
	default:
		DCPOMATIC_ASSERT (false);
	}

	DCPOMATIC_ASSERT (false);
}

libdcp::FrameInfo
read_frame_info (FILE* file, int frame, Eyes eyes)
{
	libdcp::FrameInfo info;
	dcpomatic_fseek (file, frame_info_position (frame, eyes), SEEK_SET);
	fread (&info.offset, sizeof (info.offset), 1, file);
	fread (&info.size, sizeof (info.size), 1, file);

	char hash_buffer[33];
	fread (hash_buffer, 1, 32, file);
	hash_buffer[32] = '\0';
	info.hash = hash_buffer;

	return info;
}

void
write_frame_info (FILE* file, int frame, Eyes eyes, libdcp::FrameInfo info)
{
	dcpomatic_fseek (file, frame_info_position (frame, eyes), SEEK_SET);
	fwrite (&info.offset, sizeof (info.offset), 1, file);
	fwrite (&info.size, sizeof (info.size), 1, file);
	fwrite (info.hash.c_str(), 1, info.hash.size(), file);
}

string
video_mxf_filename (shared_ptr<libdcp::Asset> asset)
{
	return "j2c_" + asset->uuid() + ".mxf";
}

string
audio_mxf_filename (shared_ptr<libdcp::Asset> asset)
{
	return "pcm_" + asset->uuid() + ".mxf";
}

ScopedTemporary::ScopedTemporary ()
	: _open (0)
{
	_file = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path ();
}

ScopedTemporary::~ScopedTemporary ()
{
	close ();
	boost::system::error_code ec;
	boost::filesystem::remove (_file, ec);
}

char const *
ScopedTemporary::c_str () const
{
	return _file.string().c_str ();
}

FILE*
ScopedTemporary::open (char const * params)
{
	_open = fopen (c_str(), params);
	return _open;
}

void
ScopedTemporary::close ()
{
	if (_open) {
		fclose (_open);
		_open = 0;
	}
}
