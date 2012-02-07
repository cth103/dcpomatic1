#include <string>
#include <inttypes.h>

struct AVFilterGraph;
struct AVCodecContext;
struct AVFilterContext;
class Format;

class Film
{
public:
	Film (std::string const &);

	void set_top_crop (int);
	void set_bottom_crop (int);
	void set_left_crop (int);
	void set_right_crop (int);
	void set_format (Format *);
	
	void make_tiffs (std::string const &, int N = 0);
	
private:
	std::pair<AVFilterContext *, AVFilterContext *> setup_filters (AVCodecContext *, std::string const &);
	void write_tiff (std::string const &, int, uint8_t *, int, int) const;
	
	std::string _content;
	Format* _format;
	int _left_crop;
	int _right_crop;
	int _top_crop;
	int _bottom_crop;
};
