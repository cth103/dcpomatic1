#include <sstream>
#include "audio_stream.h"

using namespace std;

AudioStream::AudioStream (int s, int c)
	: _sample_rate (s)
	, _channels (c)
{

}

AudioStream::AudioStream (string const & m)
{
	stringstream s (m);
	s >> _sample_rate >> _channels;
}

string
AudioStream::get_as_metadata () const
{
	stringstream s;
	s << _sample_rate << " " << _channels;
	return s.str ();
}

string
AudioStream::get_as_description () const
{
	stringstream s;
	s << _sample_rate << "Hz, " << _channels << " channels";
	return s.str ();
}
