#include "encoder.h"

Encoder::Encoder ()
	: _decoder (0)
{

}

void
Encoder::set_decoder (Decoder* d)
{
	_decoder = d;
}
