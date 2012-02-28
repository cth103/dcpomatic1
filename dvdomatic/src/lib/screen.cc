#include "screen.h"
#include "format.h"
#include "exceptions.h"

using namespace std;

vector<Screen const *> Screen::_screens;

Screen::Screen (string n)
	: _name (n)
{

}

void
Screen::add_geometry (Format const * f, Position p, Size s)
{
	_geometries.insert (make_pair (f, Geometry (p, s)));
}

void
Screen::setup_screens ()
{
	Screen* s = new Screen ("Laptop");
	s->add_geometry (Format::get_from_ratio (137), Position (0, 0), Size (1024, 747));
	s->add_geometry (Format::get_from_ratio (185), Position (0, 0), Size (1024, 554));
	s->add_geometry (Format::get_from_ratio (239), Position (0, 0), Size (1024, 428));
	_screens.push_back (s);
}

vector<Screen const *>
Screen::get_all ()
{
	return _screens;
}

Position
Screen::position (Format const * f) const
{
	GeometryMap::const_iterator i = _geometries.find (f);
	if (i == _geometries.end ()) {
		throw PlayError ("format not found for screen");
	}

	return i->second.position;
}

Size
Screen::size (Format const * f) const
{
	GeometryMap::const_iterator i = _geometries.find (f);
	if (i == _geometries.end ()) {
		throw PlayError ("format not found for screen");
	}

	return i->second.size;
}

Screen const *
Screen::get_from_index (int i)
{
	assert (i <= int(_screens.size ()));
	return _screens[i];
}
