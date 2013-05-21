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

/** @file  src/film.h
 *  @brief A representation of some audio and video content, and details of
 *  how they should be presented in a DCP.
 */

#ifndef DCPOMATIC_FILM_H
#define DCPOMATIC_FILM_H

#include <string>
#include <vector>
#include <inttypes.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/signals2.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "util.h"
#include "dci_metadata.h"
#include "types.h"
#include "ffmpeg_content.h"
#include "playlist.h"

class DCPContentType;
class Format;
class Job;
class Filter;
class Log;
class ExamineContentJob;
class AnalyseAudioJob;
class ExternalAudioStream;
class Content;
class Player;

/** @class Film
 *  @brief A representation of some audio and video content, and details of
 *  how they should be presented in a DCP.
 */
class Film : public boost::enable_shared_from_this<Film>
{
public:
	Film (std::string d, bool must_exist = true);
	Film (Film const &);

	std::string info_dir () const;
	std::string j2c_path (int f, bool t) const;
	std::string info_path (int f) const;
	std::string internal_video_mxf_dir () const;
	std::string internal_video_mxf_filename () const;
	std::string audio_analysis_path () const;

	void examine_content (boost::shared_ptr<Content>);
	std::string dcp_video_mxf_filename () const;
	std::string dcp_audio_mxf_filename () const;

	void analyse_audio ();
	void send_dcp_to_tms ();
	void make_dcp ();

	/** @return Logger.
	 *  It is safe to call this from any thread.
	 */
	boost::shared_ptr<Log> log () const {
		return _log;
	}

	int encoded_frames () const;
	
	std::string file (std::string f) const;
	std::string dir (std::string d) const;

	void write_metadata () const;

	libdcp::Size cropped_size (libdcp::Size) const;
	std::string dci_name (bool if_created_now) const;
	std::string dcp_name (bool if_created_now = false) const;

	/** @return true if our state has changed since we last saved it */
	bool dirty () const {
		return _dirty;
	}

	bool have_dcp () const;

	boost::shared_ptr<Player> player () const;
	boost::shared_ptr<Playlist> playlist () const;

	OutputAudioFrame dcp_audio_frame_rate () const;

	OutputAudioFrame time_to_audio_frames (Time) const;
	OutputVideoFrame time_to_video_frames (Time) const;
	Time video_frames_to_time (OutputVideoFrame) const;
	Time audio_frames_to_time (OutputAudioFrame) const;

	/* Proxies for some Playlist methods */

	Playlist::ContentList content () const;

	Time length () const;
	bool has_subtitles () const;
	OutputVideoFrame best_dcp_video_frame_rate () const;

	void set_loop (int);
	int loop () const;

	/** Identifiers for the parts of our state;
	    used for signalling changes.
	*/
	enum Property {
		NONE,
		NAME,
		USE_DCI_NAME,
		/** The playlist's content list has changed (i.e. content has been added, moved around or removed) */
		CONTENT,
		LOOP,
		DCP_CONTENT_TYPE,
		FORMAT,
		CROP,
		FILTERS,
		SCALER,
		AB,
		AUDIO_GAIN,
		AUDIO_DELAY,
		WITH_SUBTITLES,
		SUBTITLE_OFFSET,
		SUBTITLE_SCALE,
		COLOUR_LUT,
		J2K_BANDWIDTH,
		DCI_METADATA,
		DCP_VIDEO_FRAME_RATE,
	};


	/* GET */

	std::string directory () const {
		boost::mutex::scoped_lock lm (_directory_mutex);
		return _directory;
	}

	std::string name () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _name;
	}

	bool use_dci_name () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _use_dci_name;
	}

	DCPContentType const * dcp_content_type () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _dcp_content_type;
	}

	Format const * format () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _format;
	}

	Crop crop () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _crop;
	}

	std::vector<Filter const *> filters () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _filters;
	}

	Scaler const * scaler () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _scaler;
	}

	bool ab () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _ab;
	}

	float audio_gain () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _audio_gain;
	}

	int audio_delay () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _audio_delay;
	}

	bool with_subtitles () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _with_subtitles;
	}

	int subtitle_offset () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _subtitle_offset;
	}

	float subtitle_scale () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _subtitle_scale;
	}

	int colour_lut () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _colour_lut;
	}

	int j2k_bandwidth () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _j2k_bandwidth;
	}

	DCIMetadata dci_metadata () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _dci_metadata;
	}

	/* XXX: -> "video_frame_rate" */
	int dcp_video_frame_rate () const {
		boost::mutex::scoped_lock lm (_state_mutex);
		return _dcp_video_frame_rate;
	}

	/* SET */

	void set_directory (std::string);
	void set_name (std::string);
	void set_use_dci_name (bool);
	void add_content (boost::shared_ptr<Content>);
	void remove_content (boost::shared_ptr<Content>);
	void set_dcp_content_type (DCPContentType const *);
	void set_format (Format const *);
	void set_crop (Crop);
	void set_left_crop (int);
	void set_right_crop (int);
	void set_top_crop (int);
	void set_bottom_crop (int);
	void set_filters (std::vector<Filter const *>);
	void set_scaler (Scaler const *);
	void set_ab (bool);
	void set_audio_gain (float);
	void set_audio_delay (int);
	void set_with_subtitles (bool);
	void set_subtitle_offset (int);
	void set_subtitle_scale (float);
	void set_colour_lut (int);
	void set_j2k_bandwidth (int);
	void set_dci_metadata (DCIMetadata);
	void set_dcp_video_frame_rate (int);
	void set_dci_date_today ();

	/** Emitted when some property has of the Film has changed */
	mutable boost::signals2::signal<void (Property)> Changed;

	/** Emitted when some property of our content has changed */
	mutable boost::signals2::signal<void (boost::weak_ptr<Content>, int)> ContentChanged;

	boost::signals2::signal<void ()> AudioAnalysisSucceeded;

	/** Current version number of the state file */
	static int const state_version;

private:
	
	void signal_changed (Property);
	void analyse_audio_finished ();
	std::string video_state_identifier () const;
	void read_metadata ();
	void playlist_changed ();
	void playlist_content_changed (boost::weak_ptr<Content>, int);
	std::string filename_safe_name () const;

	/** Log to write to */
	boost::shared_ptr<Log> _log;
	/** Any running AnalyseAudioJob, or 0 */
	boost::shared_ptr<AnalyseAudioJob> _analyse_audio_job;
	boost::shared_ptr<Playlist> _playlist;

	/** Complete path to directory containing the film metadata;
	 *  must not be relative.
	 */
	std::string _directory;
	/** Mutex for _directory */
	mutable boost::mutex _directory_mutex;
	
	/** Name for DCP-o-matic */
	std::string _name;
	/** True if a auto-generated DCI-compliant name should be used for our DCP */
	bool _use_dci_name;
	/** The type of content that this Film represents (feature, trailer etc.) */
	DCPContentType const * _dcp_content_type;
	/** The format to present this Film in (flat, scope, etc.) */
	Format const * _format;
	/** The crop to apply to the source */
	Crop _crop;
	/** Video filters that should be used when generating DCPs */
	std::vector<Filter const *> _filters;
	/** Scaler algorithm to use */
	Scaler const * _scaler;
	/** true to create an A/B comparison DCP, where the left half of the image
	    is the video without any filters or post-processing, and the right half
	    has the specified filters and post-processing.
	*/
	bool _ab;
	/** Gain to apply to audio in dB */
	float _audio_gain;
	/** Delay to apply to audio (positive moves audio later) in milliseconds */
	int _audio_delay;
	/** True if subtitles should be shown for this film */
	bool _with_subtitles;
	/** y offset for placing subtitles, in source pixels; +ve is further down
	    the frame, -ve is further up.
	*/
	int _subtitle_offset;
	/** scale factor to apply to subtitles */
	float _subtitle_scale;
	/** index of colour LUT to use when converting RGB to XYZ.
	 *  0: sRGB
	 *  1: Rec 709
	 */
	int _colour_lut;
	/** bandwidth for J2K files in bits per second */
	int _j2k_bandwidth;
	/** DCI naming stuff */
	DCIMetadata _dci_metadata;
	/** Frames per second to run our DCP at */
	int _dcp_video_frame_rate;
	/** The date that we should use in a DCI name */
	boost::gregorian::date _dci_date;

	/** true if our state has changed since we last saved it */
	mutable bool _dirty;

	/** Mutex for all state except _directory */
	mutable boost::mutex _state_mutex;

	friend class paths_test;
	friend class film_metadata_test;
};

#endif
