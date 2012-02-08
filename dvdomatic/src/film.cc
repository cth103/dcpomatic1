#include <stdexcept>
#include "film.h"
#include "format.h"

using namespace std;

Film::Film (string const & c)
	: _content (c)
	, _format (0)
	, _left_crop (0)
	, _right_crop (0)
	, _top_crop (0)
	, _bottom_crop (0)
{

}

void
Film::set_top_crop (int c)
{
	_top_crop = c;
}

void
Film::set_bottom_crop (int c)
{
	_bottom_crop = c;
}

void
Film::set_left_crop (int c)
{
	_left_crop = c;
}

void
Film::set_right_crop (int c)
{
	_right_crop = c;
}

void
Film::set_format (Format* f)
{
	_format = f;
}
