#include <string>
#include <gtkmm.h>

class Progress;

class ProgressDialog : public Gtk::Dialog
{
public:
	ProgressDialog (Progress *, std::string const &);
	void spin ();
	
private:
	void update ();
	
	Gtk::ProgressBar _bar;
	Progress* _progress;
};
