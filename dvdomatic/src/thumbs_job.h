#include "job.h"

class ThumbsJob : public Job
{
public:
	ThumbsJob (Film *);
	std::string name () const;
	void run ();
};
