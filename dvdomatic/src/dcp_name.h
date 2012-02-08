
/* From http://digitalcinemanamingconvention.com/ */

namespace DCP
{

class ContentType
{
public:
	enum Type {
		FEATURE,
		TRAILER,
		RATING,
		TEASER,
		POLICY,
		PUBLIC_SERVICE_ANNOUNCEMENT,
		ADVERTISEMENT,
		SHORT,
		TRANSITIONAL,
		TEST
	};

	enum Dimensions {
		JUST_2D,
		TWOD_OF_3D,
		THREED
	};

	ContentType (Type);
	ContentType (Type, int, Dimensions, Type);

	std::string get () const;

private:
	Type _type;
	int _version;
	Dimensions _dimensions;
	Type _rating_for;
};

enum AspectRatio {
	FLAT,
	SCOPE,
	FULL
};

enum Rating {
	U,
	PG,
	TWELVEA,
	FIFTEEN,
	EIGHTEEN,
	EIGHTEEN_R
};

enum Audio {
	FIVE_POINT_ONE,
	SIX_POINT_ONE,
	SEVEN_POINT_ONE,
	MONO,
	STEREO,
	FIVE_POINT_ONE_HI_VI,
	FIVE_POINT_ONE_HI,
	FIVE_POINT_ONE_VI
};

}

std::string dcp_name (std::string const &, DCP::ContentType, DCP::AspectRatio, DCP::Rating, DCP::Audio);
