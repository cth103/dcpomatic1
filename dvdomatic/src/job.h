#include <string>
#include "progress.h"

class Film;

class Job
{
public:
	Job (Film *);

	virtual std::string name () const = 0;
	virtual void run () = 0;
	
	void start ();

	bool running () const;
	bool finished () const;
	bool finished_ok () const;
	bool finished_in_error () const;
	float progress () const;

protected:

	enum State {
		NEW,
		RUNNING,
		FINISHED_OK,
		FINISHED_ERROR
	};
	
	void set_state (State);
	
	Film* _film;
	Progress _progress;
	std::string _name;

private:
	
	mutable boost::mutex _state_mutex;
	State _state;
};
