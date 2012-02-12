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
extern "C" {
#include <libavcodec/avcodec.h>
}

struct AVFilterGraph;
struct AVCodecContext;
struct AVFilterContext;
struct AVFormatContext;
struct AVFrame;
struct AVBufferContext;
struct AVCodec;
class Film;
class Job;

class Transcoder
{
public:
	Transcoder (Film const *, Job *, int, int, int N = 0);
	~Transcoder ();

	void decode_video (bool);
	void set_decode_video_period (int);
	void decode_audio (bool);
	void apply_crop (bool);
	
	int length_in_frames () const;
	float frames_per_second () const;
	int native_width () const;
	int native_height () const;

	void go ();
	
protected:

	virtual void process_begin () {}
	virtual void process_video (uint8_t *, int) {} 
	virtual void process_audio (uint8_t *, int, int) {}
	virtual void process_end () {}

	enum PassResult {
		PASS_DONE,
		PASS_NOTHING,
		PASS_ERROR,   ///< error; probably temporary
		PASS_VIDEO,
		PASS_AUDIO
	};
	
	PassResult pass ();

	bool have_video_stream () const {
		return _video_stream != -1;
	}

	int audio_channels () const;
	int audio_sample_rate () const;
	AVSampleFormat audio_sample_format () const;

	int video_frame () const {
		return _video_frame;
	}

	int out_width () const {
		return _out_width;
	}

	int out_height () const {
		return _out_height;
	}

	Film const * _film;
	
private:

	void setup_general ();
	void setup_video ();
	void setup_video_filters (std::string const &);
	void setup_audio ();

	Job* _job;
	int _nframes;
	
	int _out_width;
	int _out_height;
	
	AVFormatContext* _format_context;
	int _video_stream;
	int _audio_stream;
	AVFrame* _frame_in;
	AVFrame* _frame_out;
	
	AVCodecContext* _video_codec_context;
	AVCodec* _video_codec;
	uint8_t* _frame_out_buffer;
	AVFilterContext* _buffer_src_context;
	AVFilterContext* _buffer_sink_context;
	struct SwsContext* _conversion_context;
	
	AVCodecContext* _audio_codec_context;
	AVCodec* _audio_codec;

	AVPacket _packet;

	bool _decode_video;
	int _decode_video_period;
	int _video_frame;
	bool _decode_audio;

	bool _apply_crop;
};
