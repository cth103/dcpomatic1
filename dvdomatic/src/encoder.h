#include <boost/shared_ptr.hpp>
#include <stdint.h>

class FilmState;
class Options;

class Encoder
{
public:
	Encoder (boost::shared_ptr<const FilmState>, boost::shared_ptr<const Options>);

	virtual void process_begin () = 0;
	virtual void process_video (uint8_t*, int, int) = 0;
	virtual void process_audio (uint8_t*, int, int) = 0;
	virtual void process_end () = 0;

protected:
	boost::shared_ptr<const FilmState> _fs;
	boost::shared_ptr<const Options> _opt;
};
