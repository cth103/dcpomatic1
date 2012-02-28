#include <string>
#include <vector>
#include <map>
#include "util.h"

class Format;

class Screen
{
public:
	Screen (std::string);

	void add_geometry (Format const *, Position, Size);

	std::string name () const {
		return _name;
	}
	
	Position position (Format const *) const;
	Size size (Format const *) const;

	static std::vector<Screen const *> get_all ();
	static Screen const * get_from_index (int);
	static void setup_screens ();

private:

	std::string _name;
	
	struct Geometry {
		Geometry (Position p, Size s)
			: position (p)
			, size (s)
		{}
		
		Position position;
		Size size;
	};

	typedef std::map<Format const *, Geometry> GeometryMap;
	GeometryMap _geometries;

	static std::vector<Screen const *> _screens;
};
