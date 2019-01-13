#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <QVector>

#include "project/marker.h"
#include "project/selection.h"

struct Clip;
class Transition;
class Media;

struct Sequence {
	Sequence();
	~Sequence();
	Sequence* copy();
	QString name;
	void getTrackLimits(int* video_tracks, int* audio_tracks);
	long getEndFrame();
	void hard_delete_transition(Clip *c, int type);
	int width;
	int height;
	double frame_rate;
	int audio_frequency;
	int audio_layout;

	QVector<Selection> selections;
	long playhead;

	bool using_workarea;
    bool enable_workarea;
	long workarea_in;
	long workarea_out;

	bool wrapper_sequence;

	int save_id;

	QVector<Marker> markers;
	QVector<Clip*> clips;
	QVector<Transition*> transitions;
};

// static variable for the currently active sequence
extern Sequence* sequence;

#endif // SEQUENCE_H
