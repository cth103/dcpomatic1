#include <string>
#include <inttypes.h>

class Format;

class Film
{
public:
	Film (std::string const &);

	void write_metadata () const;

	std::string content () const;

	int top_crop () const {
		return _top_crop;
	}

	int bottom_crop () const {
		return _bottom_crop;
	}

	int left_crop () const {
		return _left_crop;
	}

	int right_crop () const {
		return _right_crop;
	}

	Format* format () const {
		return _format;
	}

	void set_content (std::string const &);
	std::string dir (std::string const &);
	void set_top_crop (int);
	void set_bottom_crop (int);
	void set_left_crop (int);
	void set_right_crop (int);
	void set_format (Format *);
	
private:
	void read_metadata ();
	std::string metadata_file () const;
	
	/** Directory containing the film metadata */
	std::string _directory;
	/** File containing content (relative to _directory) */
	std::string _content;
	Format* _format;
	int _left_crop;
	int _right_crop;
	int _top_crop;
	int _bottom_crop;
};
