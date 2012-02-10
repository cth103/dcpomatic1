#include "opendcp_job.h"

class MakeVideoMXFJob : public OpenDCPJob
{
public:
	MakeVideoMXFJob (Film *);

	std::string name () const;
	void run ();
};

