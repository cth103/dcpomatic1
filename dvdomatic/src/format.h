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

	std::string nickname () const {
		return _nickname;
	}

	std::string get_as_metadata () const;
	
	static Format * get_from_ratio (float);
	static Format * get_from_nickname (std::string const &);
	static Format * get_from_metadata (std::string const &);
	static void setup_formats ();
	
private:

	float _ratio;
	int _dci_width;
	int _dci_height;
	std::string _nickname;

	static std::list<Format *> _formats;
};

	
