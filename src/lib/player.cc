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

#include <stdint.h>
#include "player.h"
#include "film.h"
#include "ffmpeg_decoder.h"
#include "ffmpeg_content.h"
#include "imagemagick_decoder.h"
#include "imagemagick_content.h"
#include "sndfile_decoder.h"
#include "sndfile_content.h"
#include "playlist.h"
#include "job.h"
#include "image.h"
#include "ratio.h"
#include "resampler.h"

using std::list;
using std::cout;
using std::min;
using std::max;
using std::vector;
using std::pair;
using std::map;
using boost::shared_ptr;
using boost::weak_ptr;
using boost::dynamic_pointer_cast;

#define DEBUG_PLAYER 1

class Piece
{
public:
	Piece (shared_ptr<Content> c)
		: content (c)
		, video_position (c->start ())
		, audio_position (c->start ())
	{}
	
	Piece (shared_ptr<Content> c, shared_ptr<Decoder> d)
		: content (c)
		, decoder (d)
		, video_position (c->start ())
		, audio_position (c->start ())
	{}
	
	shared_ptr<Content> content;
	shared_ptr<Decoder> decoder;
	Time video_position;
	Time audio_position;
};

#ifdef DEBUG_PLAYER
std::ostream& operator<<(std::ostream& s, Piece const & p)
{
	if (dynamic_pointer_cast<FFmpegContent> (p.content)) {
		s << "\tffmpeg     ";
	} else if (dynamic_pointer_cast<ImageMagickContent> (p.content)) {
		s << "\timagemagick";
	} else if (dynamic_pointer_cast<SndfileContent> (p.content)) {
		s << "\tsndfile    ";
	}
	
	s << " at " << p.content->start() << " until " << p.content->end();
	
	return s;
}
#endif	

Player::Player (shared_ptr<const Film> f, shared_ptr<const Playlist> p)
	: _film (f)
	, _playlist (p)
	, _video (true)
	, _audio (true)
	, _have_valid_pieces (false)
	, _video_position (0)
	, _audio_position (0)
	, _audio_buffers (f->dcp_audio_channels(), 0)
{
	_playlist->Changed.connect (bind (&Player::playlist_changed, this));
	_playlist->ContentChanged.connect (bind (&Player::content_changed, this, _1, _2));
	_film->Changed.connect (bind (&Player::film_changed, this, _1));
	set_video_container_size (_film->container()->size (_film->full_frame ()));
}

void
Player::disable_video ()
{
	_video = false;
}

void
Player::disable_audio ()
{
	_audio = false;
}

bool
Player::pass ()
{
	if (!_have_valid_pieces) {
		setup_pieces ();
		_have_valid_pieces = true;
	}

#ifdef DEBUG_PLAYER
	cout << "= PASS " << this << "\n";
#endif	

        Time earliest_t = TIME_MAX;
        shared_ptr<Piece> earliest;
	enum {
		VIDEO,
		AUDIO
	} type = VIDEO;

	for (list<shared_ptr<Piece> >::iterator i = _pieces.begin(); i != _pieces.end(); ++i) {
		if ((*i)->decoder->done ()) {
			continue;
		}

		if (dynamic_pointer_cast<VideoDecoder> ((*i)->decoder)) {
			if ((*i)->video_position < earliest_t) {
				earliest_t = (*i)->video_position;
				earliest = *i;
				type = VIDEO;
			}
		}

		if (dynamic_pointer_cast<AudioDecoder> ((*i)->decoder)) {
			if ((*i)->audio_position < earliest_t) {
				earliest_t = (*i)->audio_position;
				earliest = *i;
				type = AUDIO;
			}
		}
	}

	if (!earliest) {
#ifdef DEBUG_PLAYER
		cout << "no earliest piece.\n";
#endif		
		
		flush ();
		return true;
	}

	switch (type) {
	case VIDEO:
		if (earliest_t > _video_position) {
#ifdef DEBUG_PLAYER
			cout << "no video here; emitting black frame.\n";
#endif
			emit_black ();
		} else {
#ifdef DEBUG_PLAYER
			cout << "Pass " << *earliest << "\n";
#endif			
			earliest->decoder->pass ();
		}
		break;

	case AUDIO:
		if (earliest_t > _audio_position) {
#ifdef DEBUG_PLAYER
			cout << "no audio here; emitting silence.\n";
#endif
			emit_silence (_film->time_to_audio_frames (earliest_t - _audio_position));
		} else {
#ifdef DEBUG_PLAYER
			cout << "Pass " << *earliest << "\n";
#endif			
			earliest->decoder->pass ();
		}
		break;
	}

#ifdef DEBUG_PLAYER
	cout << "\tpost pass " << _video_position << " " << _audio_position << "\n";
#endif	

        return false;
}

void
Player::process_video (weak_ptr<Piece> weak_piece, shared_ptr<const Image> image, bool same, VideoContent::Frame frame)
{
	shared_ptr<Piece> piece = weak_piece.lock ();
	if (!piece) {
		return;
	}

	shared_ptr<VideoContent> content = dynamic_pointer_cast<VideoContent> (piece->content);
	assert (content);

	FrameRateConversion frc (content->video_frame_rate(), _film->dcp_video_frame_rate());
	if (frc.skip && (frame % 2) == 1) {
		return;
	}

	image = image->crop (content->crop(), true);

	libdcp::Size const image_size = content->ratio()->size (_video_container_size);
	
	image = image->scale_and_convert_to_rgb (image_size, _film->scaler(), true);

#if 0	
	if (film->with_subtitles ()) {
		shared_ptr<Subtitle> sub;
		if (_timed_subtitle && _timed_subtitle->displayed_at (t)) {
			sub = _timed_subtitle->subtitle ();
		}
		
		if (sub) {
			dcpomatic::Rect const tx = subtitle_transformed_area (
				float (image_size.width) / content->video_size().width,
				float (image_size.height) / content->video_size().height,
				sub->area(), film->subtitle_offset(), film->subtitle_scale()
				);
			
			shared_ptr<Image> im = sub->image()->scale (tx.size(), film->scaler(), true);
			image->alpha_blend (im, tx.position());
		}
	}
#endif	

	if (image_size != _video_container_size) {
		assert (image_size.width <= _video_container_size.width);
		assert (image_size.height <= _video_container_size.height);
		shared_ptr<Image> im (new SimpleImage (PIX_FMT_RGB24, _video_container_size, true));
		im->make_black ();
		im->copy (image, Position ((_video_container_size.width - image_size.width) / 2, (_video_container_size.height - image_size.height) / 2));
		image = im;
	}

	Time time = content->start() + (frame * frc.factor() * TIME_HZ / _film->dcp_video_frame_rate());
	
        Video (image, same, time);
	time += TIME_HZ / _film->dcp_video_frame_rate();

	if (frc.repeat) {
		Video (image, true, time);
		time += TIME_HZ / _film->dcp_video_frame_rate();
	}

	_video_position = piece->video_position = time;
}

void
Player::process_audio (weak_ptr<Piece> weak_piece, shared_ptr<const AudioBuffers> audio, AudioContent::Frame frame)
{
	shared_ptr<Piece> piece = weak_piece.lock ();
	if (!piece) {
		return;
	}

	shared_ptr<AudioContent> content = dynamic_pointer_cast<AudioContent> (piece->content);
	assert (content);

	if (content->content_audio_frame_rate() != content->output_audio_frame_rate()) {
		audio = resampler(content)->run (audio);
	}

	/* Remap channels */
	shared_ptr<AudioBuffers> dcp_mapped (new AudioBuffers (_film->dcp_audio_channels(), audio->frames()));
	dcp_mapped->make_silent ();
	list<pair<int, libdcp::Channel> > map = content->audio_mapping().content_to_dcp ();
	for (list<pair<int, libdcp::Channel> >::iterator i = map.begin(); i != map.end(); ++i) {
		dcp_mapped->accumulate_channel (audio.get(), i->first, i->second);
	}

        /* The time of this audio may indicate that some of our buffered audio is not going to
           be added to any more, so it can be emitted.
        */

	Time const time = content->start() + (frame * TIME_HZ / _film->dcp_audio_frame_rate());

        if (time > _audio_position) {
                /* We can emit some audio from our buffers */
                OutputAudioFrame const N = _film->time_to_audio_frames (time - _audio_position);
		assert (N <= _audio_buffers.frames());
                shared_ptr<AudioBuffers> emit (new AudioBuffers (_audio_buffers.channels(), N));
                emit->copy_from (&_audio_buffers, N, 0, 0);
                Audio (emit, _audio_position);
                _audio_position = piece->audio_position = time + _film->audio_frames_to_time (N);

                /* And remove it from our buffers */
                if (_audio_buffers.frames() > N) {
                        _audio_buffers.move (N, 0, _audio_buffers.frames() - N);
                }
                _audio_buffers.set_frames (_audio_buffers.frames() - N);
        }

        /* Now accumulate the new audio into our buffers */
        _audio_buffers.ensure_size (_audio_buffers.frames() + audio->frames());
        _audio_buffers.accumulate_frames (audio.get(), 0, 0, audio->frames ());
	_audio_buffers.set_frames (_audio_buffers.frames() + audio->frames());
}

void
Player::flush ()
{
	if (_audio_buffers.frames() > 0) {
		shared_ptr<AudioBuffers> emit (new AudioBuffers (_audio_buffers.channels(), _audio_buffers.frames()));
		emit->copy_from (&_audio_buffers, _audio_buffers.frames(), 0, 0);
		Audio (emit, _audio_position);
		_audio_position += _film->audio_frames_to_time (_audio_buffers.frames ());
		_audio_buffers.set_frames (0);
	}

	while (_video_position < _audio_position) {
		emit_black ();
	}

	while (_audio_position < _video_position) {
		emit_silence (_film->time_to_audio_frames (_video_position - _audio_position));
	}
	
}

/** @return true on error */
void
Player::seek (Time t, bool accurate)
{
	if (!_have_valid_pieces) {
		setup_pieces ();
		_have_valid_pieces = true;
	}

	if (_pieces.empty ()) {
		return;
	}

	for (list<shared_ptr<Piece> >::iterator i = _pieces.begin(); i != _pieces.end(); ++i) {
		shared_ptr<VideoContent> vc = dynamic_pointer_cast<VideoContent> ((*i)->content);
		if (!vc) {
			continue;
		}
		
		Time s = t - vc->start ();
		s = max (static_cast<Time> (0), s);
		s = min (vc->length(), s);

		FrameRateConversion frc (vc->video_frame_rate(), _film->dcp_video_frame_rate());
		VideoContent::Frame f = s * vc->video_frame_rate() / (frc.factor() * TIME_HZ);
		dynamic_pointer_cast<VideoDecoder>((*i)->decoder)->seek (f, accurate);
	}

	/* XXX: don't seek audio because we don't need to... */
}

void
Player::setup_pieces ()
{
	list<shared_ptr<Piece> > old_pieces = _pieces;

	_pieces.clear ();

	Playlist::ContentList content = _playlist->content ();
	sort (content.begin(), content.end(), ContentSorter ());

	for (Playlist::ContentList::iterator i = content.begin(); i != content.end(); ++i) {

		shared_ptr<Piece> piece (new Piece (*i));

                /* XXX: into content? */

		shared_ptr<const FFmpegContent> fc = dynamic_pointer_cast<const FFmpegContent> (*i);
		if (fc) {
			shared_ptr<FFmpegDecoder> fd (new FFmpegDecoder (_film, fc, _video, _audio));
			
			fd->Video.connect (bind (&Player::process_video, this, piece, _1, _2, _3));
			fd->Audio.connect (bind (&Player::process_audio, this, piece, _1, _2));

			piece->decoder = fd;
		}
		
		shared_ptr<const ImageMagickContent> ic = dynamic_pointer_cast<const ImageMagickContent> (*i);
		if (ic) {
			shared_ptr<ImageMagickDecoder> id;
			
			/* See if we can re-use an old ImageMagickDecoder */
			for (list<shared_ptr<Piece> >::const_iterator j = old_pieces.begin(); j != old_pieces.end(); ++j) {
				shared_ptr<ImageMagickDecoder> imd = dynamic_pointer_cast<ImageMagickDecoder> ((*j)->decoder);
				if (imd && imd->content() == ic) {
					id = imd;
				}
			}

			if (!id) {
				id.reset (new ImageMagickDecoder (_film, ic));
				id->Video.connect (bind (&Player::process_video, this, piece, _1, _2, _3));
			}

			piece->decoder = id;
		}

		shared_ptr<const SndfileContent> sc = dynamic_pointer_cast<const SndfileContent> (*i);
		if (sc) {
			shared_ptr<AudioDecoder> sd (new SndfileDecoder (_film, sc));
			sd->Audio.connect (bind (&Player::process_audio, this, piece, _1, _2));

			piece->decoder = sd;
		}

		_pieces.push_back (piece);
	}

#ifdef DEBUG_PLAYER
	cout << "=== Player setup:\n";
	for (list<shared_ptr<Piece> >::iterator i = _pieces.begin(); i != _pieces.end(); ++i) {
		cout << *(i->get()) << "\n";
	}
#endif	
}

void
Player::content_changed (weak_ptr<Content> w, int p)
{
	shared_ptr<Content> c = w.lock ();
	if (!c) {
		return;
	}

	if (
		p == ContentProperty::START || p == ContentProperty::LENGTH ||
		p == VideoContentProperty::VIDEO_CROP || p == VideoContentProperty::VIDEO_RATIO
		) {
		
		_have_valid_pieces = false;
		Changed ();
	}
}

void
Player::playlist_changed ()
{
	_have_valid_pieces = false;
	Changed ();
}

void
Player::set_video_container_size (libdcp::Size s)
{
	_video_container_size = s;
	_black_frame.reset (new SimpleImage (PIX_FMT_RGB24, _video_container_size, true));
	_black_frame->make_black ();
}

shared_ptr<Resampler>
Player::resampler (shared_ptr<AudioContent> c)
{
	map<shared_ptr<AudioContent>, shared_ptr<Resampler> >::iterator i = _resamplers.find (c);
	if (i != _resamplers.end ()) {
		return i->second;
	}
	
	shared_ptr<Resampler> r (new Resampler (c->content_audio_frame_rate(), c->output_audio_frame_rate(), c->audio_channels()));
	_resamplers[c] = r;
	return r;
}

void
Player::emit_black ()
{
	/* XXX: use same here */
	Video (_black_frame, false, _video_position);
	_video_position += _film->video_frames_to_time (1);
}

void
Player::emit_silence (OutputAudioFrame most)
{
	OutputAudioFrame N = min (most, _film->dcp_audio_frame_rate() / 2);
	shared_ptr<AudioBuffers> silence (new AudioBuffers (_film->dcp_audio_channels(), N));
	silence->make_silent ();
	Audio (silence, _audio_position);
	_audio_position += _film->audio_frames_to_time (N);
}

void
Player::film_changed (Film::Property p)
{
	/* Here we should notice Film properties that affect our output, and
	   alert listeners that our output now would be different to how it was
	   last time we were run.
	*/

	if (
		p == Film::SCALER || p == Film::WITH_SUBTITLES ||
		p == Film::SUBTITLE_SCALE || p == Film::SUBTITLE_OFFSET ||
		p == Film::CONTAINER
		) {
		
		Changed ();
	}
}