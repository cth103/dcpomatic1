#include <sndfile.h>
#include "transcoder.h"

class Progress;

class J2KWAVTranscoder : public Transcoder
{
public:
	J2KWAVTranscoder (Film *, Progress *, int, int);
	~J2KWAVTranscoder ();

private:

	void process_video (uint8_t *, int);
	void process_audio (uint8_t *, int, int);

	std::vector<SNDFILE*> _sound_files;
	int _deinterleave_buffer_size;
	uint8_t* _deinterleave_buffer;
};
