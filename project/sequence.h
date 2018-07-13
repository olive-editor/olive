#ifndef SEQUENCE_H
#define SEQUENCE_H

#define UNDO_LIMIT 100

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
	float frame_rate;
	int audio_frequency;
    int audio_layout;
private:
    QVector<Clip*> clips;
};

// static variable for the currently active sequence
extern Sequence* sequence;

#endif // SEQUENCE_H
