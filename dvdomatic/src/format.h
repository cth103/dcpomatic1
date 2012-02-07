#include <string>
#include <list>

class Format
{
public:
	Format (float, int, int, std::string const &);

	float ratio () const {
		return _ratio;
	}

	int dci_width () const {
		return _dci_width;
	}

	int dci_height () const {
		return _dci_height;
	}
	
	static Format * get (float);
	static void setup_formats ();
	
private:

	float _ratio;
	int _dci_width;
	int _dci_height;
	std::string _nickname;

	static std::list<Format *> _formats;
};

	
