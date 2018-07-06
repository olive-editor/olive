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
    void delete_clip(int i);
    int split_clip(int pre, long frame);
	void get_track_limits(int* video_tracks, int* audio_tracks);
	long getEndFrame();
	int width;
	int height;
	float frame_rate;
	int audio_frequency;
	int audio_layout;

    void undo_add_current();
    void undo();
    void redo();
private:
    QVector<Clip*> clips;

    void set_undo();
    QVector<QVector<Clip*>*> undo_stack;
    int undo_pointer;
};

// static variable for the currently active sequence
extern Sequence* sequence;

#endif // SEQUENCE_H
