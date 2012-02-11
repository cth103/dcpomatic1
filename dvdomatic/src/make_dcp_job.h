#include "opendcp_job.h"

class MakeDCPJob : public OpenDCPJob
{
public:
	MakeDCPJob (Film *);

	std::string name () const;
	void run ();
};

