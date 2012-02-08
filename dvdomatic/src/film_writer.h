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

class FilmWriter
{
public:
	FilmWriter (Film *, std::string const &, std::string const &, int N = 0);
	~FilmWriter ();

private:

	void setup_general ();
	void setup_video ();
	void setup_video_filters (std::string const &);
	void setup_audio ();
	void decode ();
	void decode_video ();
	void decode_audio ();
	void write_tiff (std::string const &, int, uint8_t *, int, int) const;
	
	Film* _film;
	std::string _tiffs;
	std::string _wavs;
	int _nframes;
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
	std::vector<SNDFILE*> _sound_files;
	int _deinterleave_buffer_size;
	uint8_t* _deinterleave_buffer;

	AVPacket _packet;
	int _frame;
};
