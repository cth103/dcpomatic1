#include "encoder.h"

using namespace boost;

Encoder::Encoder (shared_ptr<const FilmState> s, shared_ptr<const Options> o)
	: _fs (s)
	, _opt (o)
{

}
