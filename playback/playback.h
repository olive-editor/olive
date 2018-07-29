#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <QVector>
#include <QMutex>

struct Clip;
struct ClipCache;
struct Sequence;
struct AVFrame;

extern bool texture_failed;

void open_clip(Clip* clip, bool multithreaded);
void cache_clip(Clip* clip, long playhead, bool write_A, bool write_B, bool reset, Clip *nest);
void close_clip(Clip* clip);
void cache_audio_worker(Clip* c, bool write_A);
void cache_video_worker(Clip* c, long playhead, ClipCache* cache);
void handle_media(Sequence* sequence, long playhead, bool multithreaded);
void reset_cache(Clip* c, long target_frame);
void get_clip_frame(Clip* c, long playhead);
float playhead_to_seconds(Clip* c, long playhead);
long seconds_to_clip_frame(Clip* c, float seconds);
float clip_frame_to_seconds(Clip* c, long clip_frame);
int retrieve_next_frame(Clip* c, AVFrame* f);
void retrieve_next_frame_raw_data(Clip* c, AVFrame* output);
bool is_clip_active(Clip* c, long playhead);
void get_next_audio(Clip* c, bool mix);
void set_sequence(Sequence* s);

struct ClipCacheData {
	Clip& clip;
	long playhead;
	bool write_A;
	bool write_B;
	bool reset;
};

#endif // PLAYBACK_H
