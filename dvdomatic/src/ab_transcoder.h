#include <stdint.h>

class Film;
class Job;
class Encoder;
class Decoder;

class ABTranscoder
{
public:
	ABTranscoder (Film *, Job *, Encoder *, int, int, int N = 0);
	~ABTranscoder ();

	void go ();

private:
	void process_video (uint8_t *, int, int, int);
	
	Film* _film;
	Job* _job;
	Encoder* _encoder;
	int _nframes;
	Decoder* _da;
	Decoder* _db;
	uint8_t* _rgb;
	int _last_frame;
};
