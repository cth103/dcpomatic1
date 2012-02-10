#include <string>
#include <gtkmm.h>

class Job;

class JobManagerView
{
public:
	JobManagerView ();

	Gtk::Widget& get_widget () {
		return _view;
	}

	void update ();

private:
	Gtk::TreeView _view;
	Glib::RefPtr<Gtk::TreeStore> _store;

	class StoreColumns : public Gtk::TreeModelColumnRecord
	{
	public:
		StoreColumns ()
		{
			add (name);
			add (job);
			add (progress);
			add (resolution);
		}

		Gtk::TreeModelColumn<std::string> name;
		Gtk::TreeModelColumn<Job*> job;
		Gtk::TreeModelColumn<float> progress;
		Gtk::TreeModelColumn<std::string> resolution;
	};

	StoreColumns _columns;
};
