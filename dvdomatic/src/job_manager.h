#include <list>
#include <boost/thread/mutex.hpp>

class Job;

class JobManager
{
public:

	void add (Job *);
	std::list<Job*> get () const;

	static JobManager* instance ();

private:
	JobManager ();
	void scheduler ();
	
	mutable boost::mutex _mutex;
	std::list<Job*> _jobs;

	static JobManager* _instance;
};
