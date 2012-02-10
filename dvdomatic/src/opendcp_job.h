#ifndef DVDOMATIC_OPENDCP_JOB_H
#define DVDOMATIC_OPENDCP_JOB_H

#include "job.h"

class OpenDCPJob : public Job
{
public:
	OpenDCPJob (Film *);

protected:
	void command (std::string const &);
};

#endif
