#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <QVector>

#include "project/clip.h"

struct Sequence {
public:
	Sequence();
	~Sequence();
    Sequence* copy();
	QString name;
    int add_clip(Clip* c);
	int clip_count();
    Clip* get_clip(int i);
    void replace_clip(int i, Clip* c);
	void get_track_limits(int* video_tracks, int* audio_tracks);
    void destroy_clip(int i, bool del);
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
private:
    QVector<Clip*> clips;
};

// static variable for the currently active sequence
extern Sequence* sequence;

#endif // SEQUENCE_H
