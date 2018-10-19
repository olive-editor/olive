#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <QVector>
#include <QMutex>

struct Clip;
struct ClipCache;
struct Sequence;
struct AVFrame;

extern bool texture_failed;
extern bool rendering;

void open_clip(Clip* clip, bool multithreaded);
void cache_clip(Clip* clip, long playhead, bool reset, Clip *nest);
void close_clip(Clip* clip);
void cache_audio_worker(Clip* c, bool write_A);
void cache_video_worker(Clip* c, long playhead);
void handle_media(Sequence* sequence, long playhead, bool multithreaded);
void reset_cache(Clip* c, long target_frame);
void get_clip_frame(Clip* c, long playhead);

long playhead_to_clip_frame(Clip* c, long playhead);
double playhead_to_clip_seconds(Clip* c, long playhead);
int64_t seconds_to_timestamp(Clip* c, double seconds);
int64_t playhead_to_timestamp(Clip* c, long playhead);

int retrieve_next_frame(Clip* c, AVFrame* f);
bool is_clip_active(Clip* c, long playhead);
void get_next_audio(Clip* c, bool mix);
void set_sequence(Sequence* s);
void closeActiveClips(Sequence* s, bool wait);

#endif // PLAYBACK_H
