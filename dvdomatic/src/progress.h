#ifndef DVDOMATIC_PROGRESS_H
#define DVDOMATIC_PROGRESS_H

#include <list>
#include <boost/thread/mutex.hpp>

class Progress
{
public:
	Progress ();
	
	void set_progress (float);

	void ascend ();
	void descend (float);

	float get_overall_progress () const;

private:
	boost::mutex _mutex;
	
	struct Level {
		Level (float a) : allocation (a), normalised (0) {}

		float allocation;
		float normalised;
	};

	std::list<Level> _stack;
};

#endif
