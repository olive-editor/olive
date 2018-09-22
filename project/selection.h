#ifndef SELECTION_H
#define SELECTION_H

struct Selection {
	long in;
	long out;
	int track;

	long old_in;
	long old_out;
	int old_track;

	bool trim_in;
};

#endif // SELECTION_H
