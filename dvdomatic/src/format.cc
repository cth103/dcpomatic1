#include "format.h"

using namespace std;

list<Format *> Format::_formats;

Format::Format (float r, int dw, int dh, string const & n)
	: _ratio (r)
	, _dci_width (dw)
	, _dci_height (dh)
	, _nickname (n)
{

}

void
Format::setup_formats ()
{
	_formats.push_back (new Format (1.85, 1998, 1080, "Flat"));
	_formats.push_back (new Format (2.39, 2048, 858, "Scope"));
	_formats.push_back (new Format (1.85, 200, 100, "Test"));
}

Format *
Format::get (float r)
{
	list<Format*>::iterator i = _formats.begin ();
	while (i != _formats.end() && (*i)->ratio() != r) {
		++i;
	}

	if (i == _formats.end ()) {
		return 0;
	}

	return *i;
}

Format *
Format::get (string const & n)
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
