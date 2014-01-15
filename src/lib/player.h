/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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
#include <libdcp/subtitle_asset.h>
#include "playlist.h"
#include "content.h"
#include "film.h"
#include "rect.h"
#include "audio_merger.h"
#include "audio_content.h"
#include "decoded.h"

class Job;
class Film;
class Playlist;
class AudioContent;
class Piece;
class Image;

/** @class PlayerImage
 *  @brief A wrapper for an Image which contains some pending operations; these may
 *  not be necessary if the receiver of the PlayerImage throws it away.
 */
class PlayerImage
{
public:
	PlayerImage (boost::shared_ptr<const Image>, Crop, libdcp::Size, libdcp::Size, Scaler const *);

	void set_subtitle (boost::shared_ptr<const Image>, Position<int>);
	
	boost::shared_ptr<Image> image (AVPixelFormat, bool);

private:
	boost::shared_ptr<const Image> _in;
	Crop _crop;
	libdcp::Size _inter_size;
	libdcp::Size _out_size;
	Scaler const * _scaler;
	boost::shared_ptr<const Image> _subtitle_image;
	Position<int> _subtitle_position;
};

class PlayerStatistics
{
public:
	struct Video {
		Video ()
			: black (0)
			, repeat (0)
			, good (0)
			, skip (0)
		{}
		
		int black;
		int repeat;
		int good;
		int skip;
	} video;

	struct Audio {
		Audio ()
			: silence (0)
			, good (0)
			, skip (0)
		{}
		
		int64_t silence;
		int64_t good;
		int64_t skip;
	} audio;

	void dump (boost::shared_ptr<Log>) const;
};
 
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
	void seek (DCPTime, bool);

	DCPTime video_position () const {
		return _video_position;
	}

	void set_video_container_size (libdcp::Size);
	void set_approximate_size ();

	bool repeat_last_video ();

	PlayerStatistics const & statistics () const;
	
	/** Emitted when a video frame is ready.
	 *  First parameter is the video image.
	 *  Second parameter is the eye(s) that should see this image.
	 *  Third parameter is the colour conversion that should be used for this image.
	 *  Fourth parameter is true if the image is the same as the last one that was emitted.
	 *  Fifth parameter is the time.
	 */
	boost::signals2::signal<void (boost::shared_ptr<PlayerImage>, Eyes, ColourConversion, bool, DCPTime)> Video;
	
	/** Emitted when some audio data is ready */
	boost::signals2::signal<void (boost::shared_ptr<const AudioBuffers>, DCPTime)> Audio;

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

	void setup_pieces ();
	void playlist_changed ();
	void content_changed (boost::weak_ptr<Content>, int, bool);
	void do_seek (DCPTime, bool);
	void flush ();
	void emit_black ();
	void emit_silence (AudioFrame);
	void film_changed (Film::Property);
	void update_subtitle_from_image ();
	void update_subtitle_from_text ();
	void emit_video (boost::weak_ptr<Piece>, boost::shared_ptr<DecodedVideo>);
	void emit_audio (boost::weak_ptr<Piece>, boost::shared_ptr<DecodedAudio>);
	void step_video_position (boost::shared_ptr<DecodedVideo>);

	boost::shared_ptr<const Film> _film;
	boost::shared_ptr<const Playlist> _playlist;
	
	bool _video;
	bool _audio;

	/** Our pieces are ready to go; if this is false the pieces must be (re-)created before they are used */
	bool _have_valid_pieces;
	std::list<boost::shared_ptr<Piece> > _pieces;

	/** The time after the last video that we emitted */
	DCPTime _video_position;
	/** The time after the last audio that we emitted */
	DCPTime _audio_position;

	AudioMerger<DCPTime, AudioFrame> _audio_merger;

	libdcp::Size _video_container_size;
	boost::shared_ptr<PlayerImage> _black_frame;

	struct {
		boost::weak_ptr<Piece> piece;
		boost::shared_ptr<DecodedImageSubtitle> subtitle;
	} _image_subtitle;

	struct {
		boost::weak_ptr<Piece> piece;
		boost::shared_ptr<DecodedTextSubtitle> subtitle;
	} _text_subtitle;
	
	struct {
		Position<int> position;
		boost::shared_ptr<Image> image;
		DCPTime from;
		DCPTime to;
	} _out_subtitle;

#ifdef DCPOMATIC_DEBUG
	boost::shared_ptr<Content> _last_video;
#endif

	bool _last_emit_was_black;

	struct {
		boost::weak_ptr<Piece> weak_piece;
		boost::shared_ptr<DecodedVideo> video;
	} _last_incoming_video;

	bool _just_did_inaccurate_seek;
	bool _approximate_size;

	PlayerStatistics _statistics;

	boost::signals2::scoped_connection _playlist_changed_connection;
	boost::signals2::scoped_connection _playlist_content_changed_connection;
	boost::signals2::scoped_connection _film_changed_connection;
};

#endif
