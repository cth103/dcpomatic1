#include <string>

class Film
{
public:
	Film (std::string const &);
	void make_tiffs (std::string const &, int N = 0);
	
private:
	std::string _content;
};
