class Progress
{
public:
	Progress ();
	
	void set_progress (int);
	void set_total (int);
	void set_done (bool);

	float get_fraction () const;
	bool get_done () const;

private:
	int _progress;
	int _total;
	int _done;
};
