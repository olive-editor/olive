#ifndef SEQUENCE_H
#define SEQUENCE_H

#define UNDO_LIMIT 100

#include <QVector>

#include "project/clip.h"

struct Sequence {
public:
	Sequence();
	~Sequence();
	QString name;
    void add_clip(Clip* c);
	int clip_count();
    Clip* get_clip(int i);
	void delete_clip(int i);
//    void delete_area(long in, long out, int track);
    Clip* split_clip(Clip* pre, long frame);
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

    void set_undo(int i);
    QVector<QVector<Clip*>*> undo_stack;
    int undo_pointer;
    int undo_stack_start;
};

// static variable for the currently active sequence
extern Sequence* sequence;

#endif // SEQUENCE_H
