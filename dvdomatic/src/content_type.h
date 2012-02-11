#include <string>
#include <vector>

class ContentType
{
public:
	ContentType (std::string const &, std::string const &);

	std::string pretty_name () {
		return _pretty_name;
	}
	
	std::string opendcp_name () {
		return _opendcp_name;
	}

	static ContentType* get_from_pretty_name (std::string const &);
	static ContentType* get_from_index (int);
	static int get_as_index (ContentType *);
	static std::vector<ContentType*> get_all ();
	static void setup_content_types ();

private:
	std::string _pretty_name;
	std::string _opendcp_name;

	static std::vector<ContentType *> _content_types;
};
     
