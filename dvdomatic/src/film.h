#ifndef DVDOMATIC_FILM_H
#define DVDOMATIC_FILM_H

#include <string>
#include <vector>
#include <inttypes.h>
#include <sigc++/signal.h>

class Format;
class Progress;

class Film
{
public:
	Film (std::string const &);

	void write_metadata () const;

	std::string directory () const {
		return _directory;
	}
	
	std::string content () const;

	std::string name () const {
		return _name;
	}
	
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

	void set_name (std::string const &);
	void set_content (std::string const &);
	std::string dir (std::string const &) const;
	std::string file (std::string const &) const;
	void set_top_crop (int);
	void set_bottom_crop (int);
	void set_left_crop (int);
	void set_right_crop (int);
	void set_format (Format *);

	int width () const {
		return _width;
	}
	
	int height () const {
		return _height;
	}

	float frames_per_second () const {
		return _frames_per_second;
	}
	
	void update_thumbs_non_gui (Progress *);
	void update_thumbs_gui ();
	int num_thumbs () const;
	int thumb_frame (int) const;
	std::string thumb_file (int) const;

	sigc::signal0<void> ThumbsChanged;
	
	enum Property {
		Name,
		LeftCrop,
		RightCrop,
		TopCrop,
		BottomCrop,
		Size,
		Content,
		FilmFormat,
		FramesPerSecond
	};

	sigc::signal1<void, Property> Changed;
	
private:
	void read_metadata ();
	std::string metadata_file () const;
	void update_dimensions ();
	
	/** Directory containing the film metadata */
	std::string _directory;
	std::string _name;
	/** File containing content (relative to _directory) */
	std::string _content;
	Format* _format;
	int _left_crop;
	int _right_crop;
	int _top_crop;
	int _bottom_crop;

	/* Data which is cached to speed things up */
	std::vector<int> _thumbs;
	int _width;
	int _height;
	float _frames_per_second;
};

#endif
