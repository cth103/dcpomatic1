#include <sigc++/signal.h>
#include "transcoder.h"
#include "encoder.h"

Transcoder::Transcoder (Film* f, Job* j, Encoder* e, int w, int h, int N)
	: _film (f)
	, _job (j)
	, _decoder (f, j, w, h, N)
{
	e->set_decoder (&_decoder);
	
	_decoder.Video.connect (sigc::mem_fun (*e, &Encoder::process_video));
	_decoder.Audio.connect (sigc::mem_fun (*e, &Encoder::process_audio));
}

void
Transcoder::go ()
{
	_decoder.go ();
}
