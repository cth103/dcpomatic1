#include <string>
#include <vector>
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
	std::string dir (std::string const &) const;
	void set_top_crop (int);
	void set_bottom_crop (int);
	void set_left_crop (int);
	void set_right_crop (int);
	void set_format (Format *);

	void update_thumbs ();
	int num_thumbs () const;
	int thumb_frame (int) const;
	std::string thumb_file (int) const;
	int thumb_width () const {
		return _thumb_width;
	}
	int thumb_height () const {
		return _thumb_height;
	}
	
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

	/* Data which is cached to speed things up */
	std::vector<int> _thumbs;
	int _thumb_width;
	int _thumb_height;
};
