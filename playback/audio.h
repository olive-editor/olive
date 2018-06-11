#ifndef AUDIO_H
#define AUDIO_H

#include <QVector>

class QAudioOutput;
class QIODevice;

struct Sequence;

extern QAudioOutput* audio_output;
extern QIODevice* audio_io_device;

#define audio_ibuffer_size 192000
extern qint8 audio_ibuffer[audio_ibuffer_size];
extern int audio_ibuffer_read;
//extern QVector<int> audio_ibuffer_write;
void clear_audio_ibuffer();

void init_audio();

#endif // AUDIO_H
