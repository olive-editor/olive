#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <QVector>

#include "project/clip.h"
#include "project/marker.h"

struct Sequence {
public:
	Sequence();
	~Sequence();
    Sequence* copy();
    QString name;
    void getTrackLimits(int* video_tracks, int* audio_tracks);
	long getEndFrame();
	int width;
	int height;
    double frame_rate;
	int audio_frequency;
    int audio_layout;

    bool using_workarea;
    long workarea_in;
    long workarea_out;

    int save_id;

    QVector<Marker> markers;
    QVector<Clip*> clips;
private:
};

// static variable for the currently active sequence
extern Sequence* sequence;

#endif // SEQUENCE_H
