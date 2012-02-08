#include <string>
#include <sstream>
#include <stdexcept>
#include "dcp_name.h"

using namespace std;

DCP::ContentType::ContentType (Type t)
	: _type (t)
	, _version (0)
	, _dimensions (JUST_2D)
	, _rating_for (FEATURE)
{
	
}

DCP::ContentType::ContentType (Type t, int v, Dimensions d, Type r)
	: _type (t)
	, _version (v)
	, _dimensions (d)
	, _rating_for (r)
{

}

string
DCP::ContentType::get () const
{
	switch (_type) {
	case FEATURE:
		return "FTR";
	case TRAILER:
	{
		stringstream s;
		s << "TLR";
		if (_version > 0) {
			s << "-" << _version;
		}
		switch (_dimensions) {
		case TWOD_OF_3D:
			s << "-2D";
			break;
		case THREED:
			s << "-3D";
			break;
		default:
			break;
		}
			
		return s.str();
	}
	case RATING:
	{
		stringstream s;
		s << "RTG";
		switch (_rating_for) {
		case FEATURE:
			s << "-F";
			break;
		case TRAILER:
			s << "-T";
			if (_version > 0) {
				s << _version;
			}
			break;
		default:
			break;
		}
		
		return s.str ();
	}
	case TEASER:
	{
		stringstream s;
		s << "TSR";
		if (_version > 0) {
			s << "-" << _version;
		}
		break;
	}
	case POLICY:
		return "POL";
	case PUBLIC_SERVICE_ANNOUNCEMENT:
		return "PSA";
	case ADVERTISEMENT:
		return "ADV";
	case SHORT:
		return "SHR";
	case TRANSITIONAL:
		return "XSN";
	case TEST:
		return "TST";
	}

	return "";
}
	

string
dcp_name (string const & film_title, DCP::ContentType content_type, DCP::AspectRatio aspect_ratio, DCP::Rating rating, DCP::Audio audio)
{
	stringstream s;
	
	if (film_title.length() > 14) {
		throw runtime_error ("Film title is too long");
	}

	if (film_title.find(' ') != string::npos) {
		throw runtime_error ("Film title cannot contain spaces");
	}
	
	s << film_title
	  << "_"
	  << content_type.get ()
	  << "_";

	switch (aspect_ratio) {
	case DCP::FLAT:
		s << "F";
		break;
	case DCP::SCOPE:
		s << "S";
		break;
	case DCP::FULL:
		s << "C";
		break;
	}

	s << "_";

	/* Not dealing with subtitles (yet) */
	s << "EN-XX"
	  << "_";

	/* Nor territories */
	s << "UK-";

	switch (rating) {
	case DCP::U:
		s << "U";
		break;
	case DCP::PG:
		s << "PG";
		break;
	case DCP::TWELVEA:
		s << "12A";
		break;
	case DCP::FIFTEEN:
		s << "15";
		break;
	case DCP::EIGHTEEN:
		s << "18";
		break;
	case DCP::EIGHTEEN_R:
		s << "18R";
		break;
	}

	s << "_";

	switch (audio) {
	case DCP::FIVE_POINT_ONE:
		s << "51";
		break;
	case DCP::SIX_POINT_ONE:
		s << "61";
		break;
	case DCP::SEVEN_POINT_ONE:
		s << "71";
		break;

	case DCP::MONO:
		s << "10";
		break;
	case DCP::STEREO:
		s << "20";
		break;
	case DCP::FIVE_POINT_ONE_HI_VI:
		s << "51-HI-VI";
		break;
	case DCP::FIVE_POINT_ONE_HI:
		s << "51-HI";
		break;
	case DCP::FIVE_POINT_ONE_VI:
		s << "51-VI";
		break;
	}

	/* Assume English language */
	s << "-EN"
	  << "_";

	/* Assume 2k */
	s << "2K"
	  << "_";
	
	/* Dummy studio name */
	s << "ST"
	  << "_";

	/* Dummy date */
	s << "20120101"
	  << "_";

	/* Dummy facility */
	s << "FAC"
	  << "_";

	/* Call it OV */
	s << "OV";

	return s.str ();
}
