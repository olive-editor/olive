#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <QVector>

#include "project/clip.h"

struct Sequence {
public:
	Sequence();
	~Sequence();
	QString name;
	Clip& new_clip();
	Clip& insert_clip(const Clip& c);
	int clip_count();
	Clip& get_clip(int i);
	void delete_clip(int i);
	void delete_area(long in, long out, int track);
	void split_clip(int i, long frame);
	void split_at_playhead(long frame);
	void get_track_limits(int* video_tracks, int* audio_tracks);
	long getEndFrame();
	int width;
	int height;
	float frame_rate;
	int audio_frequency;
	int audio_layout;
private:
	QList<Clip> clips;
};

#endif // SEQUENCE_H
