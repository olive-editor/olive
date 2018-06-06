#ifndef AUDIO_H
#define AUDIO_H

#include <QIODevice>
#include <QMutex>

struct Sequence;

extern QIODevice* audio_io_device;

extern uint8_t* audio_cache_A;
extern uint8_t* audio_cache_B;
extern int audio_cache_size;
extern int audio_bytes_written;
extern bool switch_audio_cache;
extern bool reading_audio_cache_A;
void init_audio(Sequence* s);
void clear_cache(bool clear_A, bool clear_B);

#endif // AUDIO_H
