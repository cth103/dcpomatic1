#ifndef DVDOMATIC_AUDIO_STREAM_H
#define DVDOMATIC_AUDIO_STREAM_H

#include <string>

class AudioStream
{
public:
	AudioStream (int, int);
	AudioStream (std::string const &);

	std::string get_as_metadata () const;
	std::string get_as_description () const;

private:
	int _sample_rate;
	int _channels;
};

#endif
