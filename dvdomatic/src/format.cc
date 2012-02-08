#include <sstream>
#include <cstdlib>
#include "format.h"

using namespace std;

list<Format *> Format::_formats;

Format::Format (int r, int dw, int dh, string const & n)
	: _ratio (r)
	, _dci_width (dw)
	, _dci_height (dh)
	, _nickname (n)
{

}

string
Format::get_as_metadata () const
{
	stringstream s;
	s << _ratio;
	return s.str ();
}

void
Format::setup_formats ()
{
	_formats.push_back (new Format (185, 1998, 1080, "Flat"));
	_formats.push_back (new Format (239, 2048, 858, "Scope"));
	_formats.push_back (new Format (185, 200, 100, "Test"));
}

Format *
Format::get_from_ratio (int r)
{
	list<Format*>::iterator i = _formats.begin ();
	while (i != _formats.end() && (*i)->ratio_as_integer() != r) {
		++i;
	}

	if (i == _formats.end ()) {
		return 0;
	}

	return *i;
}

Format *
Format::get_from_nickname (string const & n)
{
	list<Format*>::iterator i = _formats.begin ();
	while (i != _formats.end() && (*i)->nickname() != n) {
		++i;
	}

	if (i == _formats.end ()) {
		return 0;
	}

	return *i;
}

Format *
Format::get_from_metadata (string const & m)
{
	return get_from_ratio (atoi (m.c_str ()));
}

