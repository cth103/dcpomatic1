/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

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

#ifndef DCPOMATIC_PLAYER_H
#define DCPOMATIC_PLAYER_H

#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "playlist.h"
#include "content.h"
#include "film.h"
#include "rect.h"
#include "audio_merger.h"
#include "audio_content.h"
#include "piece.h"
#include "subtitle.h"

class Job;
class Film;
class Playlist;
class AudioContent;
class Piece;
class Image;
class Resampler;
class PlayerVideoFrame;
class ImageProxy;
 
/** @class Player
 *  @brief A class which can `play' a Playlist; emitting its audio and video.
 */
class Player : public boost::enable_shared_from_this<Player>, public boost::noncopyable
{
public:
	Player (boost::shared_ptr<const Film>, boost::shared_ptr<const Playlist>);

	void disable_video ();
	void disable_audio ();

	bool pass ();
	void seek (Time, bool);

	Time video_position () const {
		return _video_position;
	}

	void set_video_container_size (libdcp::Size);

	bool repeat_last_video ();

	/** Emitted when a video frame is ready.
	 *  First parameter is the video image.
	 *  Second parameter is true if the frame is the same as the last one that was emitted.
	 *  Third parameter is the time.
	 */
	boost::signals2::signal<void (boost::shared_ptr<PlayerVideoFrame>, bool, Time)> Video;
	
	/** Emitted when some audio data is ready */
	boost::signals2::signal<void (boost::shared_ptr<const AudioBuffers>, Time)> Audio;

	/** Emitted when something has changed such that if we went back and emitted
	 *  the last frame again it would look different.  This is not emitted after
	 *  a seek.
	 *
	 *  The parameter is true if these signals are currently likely to be frequent.
	 */
	boost::signals2::signal<void (bool)> Changed;

private:
	friend class PlayerWrapper;
	friend class Piece;

	void process_video (boost::weak_ptr<Piece>, boost::shared_ptr<const ImageProxy>, Eyes, Part, bool, VideoContent::Frame, Time);
	void process_audio (boost::weak_ptr<Piece>, boost::shared_ptr<const AudioBuffers>, AudioContent::Frame, bool);
	void process_subtitle (boost::weak_ptr<Piece>, boost::shared_ptr<Image>, dcpomatic::Rect<double>, Time, Time);
	void setup_pieces ();
	void playlist_changed ();
	void content_changed (boost::weak_ptr<Content>, int, bool);
	void do_seek (Time, bool);
	void flush ();
	void emit_black ();
	void emit_silence (OutputAudioFrame);
	boost::shared_ptr<Resampler> resampler (boost::shared_ptr<AudioContent>, bool);
	void film_changed (Film::Property);
	void update_subtitle ();

	boost::shared_ptr<const Film> _film;
	boost::shared_ptr<const Playlist> _playlist;
	
	bool _video;
	bool _audio;

	/** Our pieces are ready to go; if this is false the pieces must be (re-)created before they are used */
	bool _have_valid_pieces;
	std::list<boost::shared_ptr<Piece> > _pieces;

	/** The time after the last video that we emitted */
	Time _video_position;
	/** The time after the last audio that we emitted */
	Time _audio_position;

	AudioMerger<Time, AudioContent::Frame> _audio_merger;

	/** Size of the image in the DCP (e.g. 1998x1080 for flat) */
	libdcp::Size _video_container_size;
	boost::shared_ptr<PlayerVideoFrame> _black_frame;
	std::map<boost::shared_ptr<AudioContent>, boost::shared_ptr<Resampler> > _resamplers;

	std::list<Subtitle> _subtitles;

#ifdef DCPOMATIC_DEBUG
	boost::shared_ptr<Content> _last_video;
#endif

	bool _last_emit_was_black;

	IncomingVideo _last_incoming_video;

	boost::signals2::scoped_connection _playlist_changed_connection;
	boost::signals2::scoped_connection _playlist_content_changed_connection;
	boost::signals2::scoped_connection _film_changed_connection;
};

#endif
