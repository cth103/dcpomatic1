#include <stdint.h>

class Decoder;
class Film;
class Parameters;

class Encoder
{
public:
	Encoder (Film const *, Parameters const *);

	virtual void process_begin () = 0;
	virtual void process_video (uint8_t*, int, int) = 0;
	virtual void process_audio (uint8_t*, int, int) = 0;
	virtual void process_end () = 0;

protected:
	Film const * _film;
	Parameters const * _par;
};
