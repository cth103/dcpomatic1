#include "decoder.h"

class Film;
class Job;
class Encoder;

class Transcoder
{
public:
	Transcoder (Film *, Job *, Encoder *, int, int, int N = 0);

	void go ();

	Decoder* decoder () {
		return &_decoder;
	}

protected:
	Film* _film;
	Job* _job;
	Encoder* _encoder;
	Decoder _decoder;
};
