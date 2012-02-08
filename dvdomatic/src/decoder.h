#include <vector>
#include <string>
extern "C" {
#include <libavcodec/avcodec.h>
}
#include <sndfile.h>

struct AVFilterGraph;
struct AVCodecContext;
struct AVFilterContext;
struct AVFormatContext;
struct AVFrame;
struct AVBufferContext;
struct AVCodec;
class Film;
class Progress;

class Decoder
{
public:
	Decoder (Film *);
	~Decoder ();

	bool have_video_stream () const {
		return _video_stream != -1;
	}

	int audio_channels () const;
	int audio_sample_rate () const;
	int length_in_frames () const;
	AVSampleFormat audio_sample_format () const;

	int pass ();
	virtual void process_video (uint8_t *) = 0;
	virtual void process_audio (uint8_t *, int, int) = 0;
	
private:

	void setup_general ();
	void setup_video ();
	void setup_video_filters (std::string const &);
	void setup_audio ();
	void decode_video ();
	void decode_audio ();
	void write_tiff (std::string const &, int, uint8_t *, int, int) const;
	
	Film* _film;
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
};
