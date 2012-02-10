#include "job.h"

class OpenDCPJob : public Job
{
public:
	OpenDCPJob (Film *);

protected:
	void command (std::string const &);
};

	
