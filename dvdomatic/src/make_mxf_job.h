#include "opendcp_job.h"

class MakeMXFJob : public OpenDCPJob
{
public:
	enum Type {
		AUDIO,
		VIDEO
	};
	
	MakeMXFJob (Film *, Type);

	std::string name () const;
	void run ();

private:
	Type _type;
};

