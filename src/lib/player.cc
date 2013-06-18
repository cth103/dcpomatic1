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
#include "null_content.h"
#include "black_decoder.h"
#include "silence_decoder.h"

using std::list;
using std::cout;
using std::min;
using std::max;
using std::vector;
using boost::shared_ptr;
using boost::weak_ptr;
using boost::dynamic_pointer_cast;

struct Piece
{
	Piece (shared_ptr<Content> c, shared_ptr<Decoder> d)
		: content (c)
		, decoder (d)
	{}
	
	shared_ptr<Content> content;
	shared_ptr<Decoder> decoder;
};

Player::Player (shared_ptr<const Film> f, shared_ptr<const Playlist> p)
	: _film (f)
	, _playlist (p)
	, _video (true)
	, _audio (true)
	, _have_valid_pieces (false)
	, _position (0)
	, _audio_buffers (f->dcp_audio_channels(), 0)
	, _next_audio (0)
{
	_playlist->Changed.connect (bind (&Player::playlist_changed, this));
	_playlist->ContentChanged.connect (bind (&Player::content_changed, this, _1, _2));
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

        /* Here we are just finding the active decoder with the earliest last emission time, then
           calling pass on it.
        */

        Time earliest_t = TIME_MAX;
        shared_ptr<Piece> earliest;

	for (list<shared_ptr<Piece> >::iterator i = _pieces.begin(); i != _pieces.end(); ++i) {
		cout << "check " << (*i)->content->file()
		     << " start=" << (*i)->content->start()
		     << ", position=" << (*i)->decoder->position()
		     << ", end=" << (*i)->content->end() << "\n";
		
		if ((*i)->decoder->done ()) {
			continue;
		}

		if (!_audio && dynamic_pointer_cast<AudioDecoder> ((*i)->decoder) && !dynamic_pointer_cast<VideoDecoder> ((*i)->decoder)) {
			continue;
		}
		
		Time const t = (*i)->content->start() + (*i)->decoder->position();
		if (t < earliest_t) {
			cout << "\t candidate; " << t << " " << (t / TIME_HZ) << ".\n";
			earliest_t = t;
			earliest = *i;
		}
	}

	if (!earliest) {
		flush ();
		return true;
	}

	cout << "PASS:\n";
	cout << "\tpass " << earliest->content->file() << " ";
	if (dynamic_pointer_cast<FFmpegContent> (earliest->content)) {
		cout << " FFmpeg.\n";
	} else if (dynamic_pointer_cast<ImageMagickContent> (earliest->content)) {
		cout << " ImageMagickContent.\n";
	} else if (dynamic_pointer_cast<SndfileContent> (earliest->content)) {
		cout << " SndfileContent.\n";
	} else if (dynamic_pointer_cast<BlackDecoder> (earliest->decoder)) {
		cout << " Black.\n";
	} else if (dynamic_pointer_cast<SilenceDecoder> (earliest->decoder)) {
		cout << " Silence.\n";
	}
	
	earliest->decoder->pass ();
	_position = earliest->content->start() + earliest->decoder->position ();
	cout << "\tpassed to " << _position << " " << (_position / TIME_HZ) << "\n";

        return false;
}

void
Player::process_video (weak_ptr<Content> weak_content, shared_ptr<const Image> image, bool same, Time time)
{
	shared_ptr<Content> content = weak_content.lock ();
	if (!content) {
		return;
	}
	
	time += content->start ();
	
        Video (image, same, time);
}

void
Player::process_audio (weak_ptr<Content> weak_content, shared_ptr<const AudioBuffers> audio, Time time)
{
	shared_ptr<Content> content = weak_content.lock ();
	if (!content) {
		return;
	}
	
        /* The time of this audio may indicate that some of our buffered audio is not going to
           be added to any more, so it can be emitted.
        */

	time += content->start ();

        if (time > _next_audio) {
                /* We can emit some audio from our buffers */
                OutputAudioFrame const N = _film->time_to_audio_frames (time - _next_audio);
		assert (N <= _audio_buffers.frames());
                shared_ptr<AudioBuffers> emit (new AudioBuffers (_audio_buffers.channels(), N));
                emit->copy_from (&_audio_buffers, N, 0, 0);
                Audio (emit, _next_audio);
                _next_audio += _film->audio_frames_to_time (N);

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
		Audio (emit, _next_audio);
		_next_audio += _film->audio_frames_to_time (_audio_buffers.frames ());
		_audio_buffers.set_frames (0);
	}
}

/** @return true on error */
void
Player::seek (Time t)
{
	if (!_have_valid_pieces) {
		setup_pieces ();
		_have_valid_pieces = true;
	}

	if (_pieces.empty ()) {
		return;
	}

//	cout << "seek to " << t << " " << (t / TIME_HZ) << "\n";

	for (list<shared_ptr<Piece> >::iterator i = _pieces.begin(); i != _pieces.end(); ++i) {
		Time s = t - (*i)->content->start ();
		s = max (static_cast<Time> (0), s);
		s = min ((*i)->content->length(), s);
//		cout << "seek [" << (*i)->content->file() << "," << (*i)->content->start() << "," << (*i)->content->end() << "] to " << s << "\n";
		(*i)->decoder->seek (s);
	}

	/* XXX: don't seek audio because we don't need to... */
}


void
Player::seek_back ()
{

}

void
Player::seek_forward ()
{

}

void
Player::add_black_piece (Time s, Time len)
{
	shared_ptr<NullContent> nc (new NullContent (_film, s, len));
	nc->set_ratio (_film->container ());
	shared_ptr<BlackDecoder> bd (new BlackDecoder (_film, nc));
	bd->Video.connect (bind (&Player::process_video, this, nc, _1, _2, _3));
	_pieces.push_back (shared_ptr<Piece> (new Piece (nc, bd)));
	cout << "\tblack @ " << s << " -- " << (s + len) << "\n";
}

void
Player::add_silent_piece (Time s, Time len)
{
	shared_ptr<NullContent> nc (new NullContent (_film, s, len));
	shared_ptr<SilenceDecoder> sd (new SilenceDecoder (_film, nc));
	sd->Audio.connect (bind (&Player::process_audio, this, nc, _1, _2));
	_pieces.push_back (shared_ptr<Piece> (new Piece (nc, sd)));
	cout << "\tsilence @ " << s << " -- " << (s + len) << "\n";
}


void
Player::setup_pieces ()
{
	cout << "----- Player SETUP PIECES.\n";

	list<shared_ptr<Piece> > old_pieces = _pieces;

	_pieces.clear ();

	Playlist::ContentList content = _playlist->content ();
	sort (content.begin(), content.end(), ContentSorter ());
	
	for (Playlist::ContentList::iterator i = content.begin(); i != content.end(); ++i) {

		shared_ptr<Decoder> decoder;
		
                /* XXX: into content? */

		shared_ptr<const FFmpegContent> fc = dynamic_pointer_cast<const FFmpegContent> (*i);
		if (fc) {
			shared_ptr<FFmpegDecoder> fd (new FFmpegDecoder (_film, fc, _video, _audio));
			
			fd->Video.connect (bind (&Player::process_video, this, *i, _1, _2, _3));
			fd->Audio.connect (bind (&Player::process_audio, this, *i, _1, _2));
			if (_video_container_size) {
				fd->set_video_container_size (_video_container_size.get ());
			}

			decoder = fd;
			cout << "\tFFmpeg @ " << fc->start() << " -- " << fc->end() << "\n";
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
				id->Video.connect (bind (&Player::process_video, this, *i, _1, _2, _3));
				if (_video_container_size) {
					id->set_video_container_size (_video_container_size.get ());
				}
			}

			decoder = id;
			cout << "\tImageMagick @ " << ic->start() << " -- " << ic->end() << "\n";
		}

		shared_ptr<const SndfileContent> sc = dynamic_pointer_cast<const SndfileContent> (*i);
		if (sc) {
			shared_ptr<AudioDecoder> sd (new SndfileDecoder (_film, sc));
			sd->Audio.connect (bind (&Player::process_audio, this, *i, _1, _2));

			decoder = sd;
			cout << "\tSndfile @ " << sc->start() << " -- " << sc->end() << "\n";
		}

		_pieces.push_back (shared_ptr<Piece> (new Piece (*i, decoder)));
	}

	/* Fill in visual gaps with black and audio gaps with silence */

	Time video_pos = 0;
	Time audio_pos = 0;
	list<shared_ptr<Piece> > pieces_copy = _pieces;
	for (list<shared_ptr<Piece> >::iterator i = pieces_copy.begin(); i != pieces_copy.end(); ++i) {
		if (dynamic_pointer_cast<VideoContent> ((*i)->content)) {
			Time const diff = (*i)->content->start() - video_pos;
			if (diff > 0) {
				add_black_piece (video_pos, diff);
			}
			video_pos = (*i)->content->end();
		}

		shared_ptr<AudioContent> ac = dynamic_pointer_cast<AudioContent> ((*i)->content);
		if (ac && ac->audio_channels()) {
			Time const diff = (*i)->content->start() - audio_pos;
			if (diff > 0) {
				add_silent_piece (video_pos, diff);
			}
			audio_pos = (*i)->content->end();
		}
	}

	if (video_pos < audio_pos) {
		add_black_piece (video_pos, audio_pos - video_pos);
	} else if (audio_pos < video_pos) {
		add_silent_piece (audio_pos, video_pos - audio_pos);
	}
}

void
Player::content_changed (weak_ptr<Content> w, int p)
{
	shared_ptr<Content> c = w.lock ();
	if (!c) {
		return;
	}

	if (p == ContentProperty::START || p == ContentProperty::LENGTH) {
		_have_valid_pieces = false;
	}
}

void
Player::playlist_changed ()
{
	_have_valid_pieces = false;
}

void
Player::set_video_container_size (libdcp::Size s)
{
	_video_container_size = s;
	for (list<shared_ptr<Piece> >::iterator i = _pieces.begin(); i != _pieces.end(); ++i) {
		shared_ptr<VideoDecoder> vd = dynamic_pointer_cast<VideoDecoder> ((*i)->decoder);
		if (vd) {
			vd->set_video_container_size (s);
		}
	}
}