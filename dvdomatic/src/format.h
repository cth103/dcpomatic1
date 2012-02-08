#include <string>
#include <list>

class Format
{
public:
	Format (int, int, int, std::string const &);

	int ratio_as_integer () const {
		return _ratio;
	}

	float ratio_as_float () const {
		return _ratio / 100.0;
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

	static Format * get_from_ratio (int);
	static Format * get_from_nickname (std::string const &);
	static Format * get_from_metadata (std::string const &);
	static void setup_formats ();
	
private:

	/** Ratio expressed as the actual ratio multiplied by 100 */
	int _ratio;
	int _dci_width;
	int _dci_height;
	std::string _nickname;

	static std::list<Format *> _formats;
};

	
