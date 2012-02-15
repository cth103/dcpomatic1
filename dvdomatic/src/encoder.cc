#include "encoder.h"

Encoder::Encoder (Parameters const * p)
	: _par (p)
	, _decoder (0)
{

}

void
Encoder::set_decoder (Decoder* d)
{
	_decoder = d;
}
