#include "opendcp_job.h"

class EncodeJob : public OpenDCPJob
{
public:
	EncodeJob (Film *);

	std::string name () const;
	void run ();
};

