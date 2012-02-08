#include "format.h"
#include "film.h"
#include "dcp_name.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE dvdomatic_test
#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_AUTO_TEST_CASE (film_metadata_test)
{
	Film f ("build/test/film");
	BOOST_CHECK(f.format() == 0);
}

BOOST_AUTO_TEST_CASE (format_test)
{
	Format::setup_formats ();
	
	Format* f = Format::get_from_nickname ("Flat");
	BOOST_CHECK (f);
	BOOST_CHECK_EQUAL (f->ratio_as_integer(), 185);
	
	f = Format::get_from_nickname ("Scope");
	BOOST_CHECK (f);
	BOOST_CHECK_EQUAL (f->ratio_as_integer(), 239);
}

BOOST_AUTO_TEST_CASE (dcp_name_test)
{
	string n;

	n = dcp_name ("THE-BLUES-BROS", DCP::ContentType (DCP::ContentType::FEATURE), DCP::FLAT, DCP::FIFTEEN, DCP::MONO);
	BOOST_CHECK_EQUAL (n, "THE-BLUES-BROS_FTR_F_EN-XX_UK-15_10-EN_2K_ST_20120101_FAC_OV");

	n = dcp_name ("LIFE-AQUATIC", DCP::ContentType (DCP::ContentType::FEATURE), DCP::SCOPE, DCP::FIFTEEN, DCP::FIVE_POINT_ONE);
	BOOST_CHECK_EQUAL (n, "LIFE-AQUATIC_FTR_S_EN-XX_UK-15_51-EN_2K_ST_20120101_FAC_OV");
}

		      
