class Progress
{
public:
	Progress ();
	
	void set_progress (int);
	void set_total (int);

	float get_fraction () const;

private:
	int _progress;
	int _total;
};
