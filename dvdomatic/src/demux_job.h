#include "job.h"

class DemuxJob : public Job
{
public:
	DemuxJob (Film *);

	std::string name () const;
	void run ();
};
