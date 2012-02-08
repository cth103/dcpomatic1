#include <vector>
#include <string>
extern "C" {
#include <libavcodec/avcodec.h>
}
#include <sndfile.h>
#include "decoder.h"

struct AVFilterGraph;
struct AVCodecContext;
struct AVFilterContext;
struct AVFormatContext;
struct AVFrame;
struct AVBufferContext;
struct AVCodec;
class Film;
class Progress;

class FilmWriter : public Decoder
{
public:
	FilmWriter (Film *, Progress *, int N = 0);
	~FilmWriter ();

	void go ();
	
private:

	void process_video (uint8_t *);
	void process_audio (uint8_t *, int, int);

	void write_tiff (std::string const &, int, uint8_t *, int, int) const;
	void decode ();
	
	std::string _tiffs;
	std::string _wavs;
	Progress* _progress;
	int _nframes;

	std::vector<SNDFILE*> _sound_files;
	int _deinterleave_buffer_size;
	uint8_t* _deinterleave_buffer;

	int _frame;
};
