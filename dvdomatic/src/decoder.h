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
#include <libpostproc/postprocess.h>
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

class Decoder
{
public:
	Decoder (Film const *, Job *, int, int, int N = 0);
	~Decoder ();

	void decode_video (bool);
	void set_decode_video_period (int);
	void decode_audio (bool);
	void apply_crop (bool);
	
	int length_in_frames () const;
	int decoding_frames () const;
	float frames_per_second () const;
	int native_width () const;
	int native_height () const;

	int out_width () const {
		return _out_width;
	}

	int out_height () const {
		return _out_height;
	}

	int audio_channels () const;
	int audio_sample_rate () const;
	AVSampleFormat audio_sample_format () const;

	Film const * film () const {
		return _film;
	}

	void go ();

	enum PassResult {
		PASS_DONE,
		PASS_NOTHING,
		PASS_ERROR,   ///< error; probably temporary
		PASS_VIDEO,
		PASS_AUDIO
	};

	PassResult pass ();
	
	sigc::signal<void, uint8_t *, int, int> Video;
	sigc::signal<void, uint8_t *, int, int> Audio;
	
protected:

	bool have_video_stream () const {
		return _video_stream != -1;
	}

	Film const * _film;
	
private:

	void setup_general ();
	void setup_video ();
	void setup_video_filters ();
	void setup_audio ();
	void setup_post_process_filters ();

	Job* _job;
	int _num_frames;
	
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

	int _post_filter_width;
	int _post_filter_height;

	/** libpostprocess mode, or 0 if no post-processing is to be done */
	pp_mode* _pp_mode;
	/** libpostprocess context, 0 if no post-processing is to be done */
	pp_context* _pp_context;
	uint8_t** _pp_buffer;
	int _pp_stride[3];
};
