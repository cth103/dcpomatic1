#include "job.h"

class EncodeJob : public Job
{
public:
	EncodeJob (Film *);

	std::string name () const;
	void run ();
};

