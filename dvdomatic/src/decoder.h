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

#include <vector>
#include <string>
#include <stdint.h>
#include <boost/shared_ptr.hpp>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libpostproc/postprocess.h>
}
#include "util.h"

struct AVFilterGraph;
struct AVCodecContext;
struct AVFilterContext;
struct AVFormatContext;
struct AVFrame;
struct AVBufferContext;
struct AVCodec;
class Job;
class FilmState;
class Options;
class Image;
class Log;

class Decoder
{
public:
	Decoder (boost::shared_ptr<const FilmState>, boost::shared_ptr<const Options>, Job *, Log *, bool minimal = false, bool ignore_length = false);
	~Decoder ();

	/* Methods to query our input video */
	int length_in_frames () const;
	int decoding_frames () const;
	float frames_per_second () const;
	Size native_size () const;
	int audio_channels () const;
	int audio_sample_rate () const;
	AVSampleFormat audio_sample_format () const;

	void go ();
	int last_video_frame () const {
		return _video_frame;
	}

	enum PassResult {
		PASS_DONE,    ///< we have decoded all the input
		PASS_NOTHING, ///< nothing new is ready after this pass
		PASS_ERROR,   ///< error; probably temporary
		PASS_VIDEO,   ///< a video frame is available
		PASS_AUDIO    ///< some audio is available
	};

	PassResult pass ();
	
	sigc::signal<void, boost::shared_ptr<Image>, int> Video;
	sigc::signal<void, uint8_t *, int, int> Audio;
	
private:

	void setup_general ();
	void setup_video ();
	void setup_video_filters ();
	void setup_audio ();

	boost::shared_ptr<const FilmState> _fs;
	boost::shared_ptr<const Options> _opt;
	Job* _job;
	Log* _log;
	
	AVFormatContext* _format_context;
	int _video_stream;
	int _audio_stream;
	AVFrame* _frame_in;
	
	AVCodecContext* _video_codec_context;
	AVCodec* _video_codec;
	AVFilterContext* _buffer_src_context;
	AVFilterContext* _buffer_sink_context;
	
	AVCodecContext* _audio_codec_context;
	AVCodec* _audio_codec;

	AVPacket _packet;

	int _video_frame;

	int _post_filter_width;
	int _post_filter_height;

	bool _minimal;
	bool _ignore_length;
};
