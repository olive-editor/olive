#ifndef AUDIO_H
#define AUDIO_H

#include <QVector>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>

//#define INT16_MAX 0x7fff
//#define INT16_MIN (-INT16_MAX-1)

class QIODevice;
class QAudioOutput;

struct Sequence;

class AudioSenderThread : public QThread {
	Q_OBJECT
public:
	AudioSenderThread();
	void run();
	void stop();
	QWaitCondition cond;
	bool close;
	QMutex lock;
public slots:
	void notifyReceiver();
private:
	QVector<qint16> samples;
	int send_audio_to_output(int offset, int max);
};

extern QAudioOutput* audio_output;
extern QIODevice* audio_io_device;
extern AudioSenderThread* audio_thread;
extern QMutex audio_write_lock;

#define audio_ibuffer_size 192000
extern qint8 audio_ibuffer[audio_ibuffer_size];
extern unsigned long audio_ibuffer_read;
extern long audio_ibuffer_frame;
extern double audio_ibuffer_timecode;
extern bool audio_scrub;
extern bool recording;
extern bool audio_rendering;
void clear_audio_ibuffer();

int current_audio_freq();

bool is_audio_device_set();

void init_audio();
void stop_audio();
unsigned long get_buffer_offset_from_frame(double framerate, long frame);

bool start_recording();
void stop_recording();
QString get_recorded_audio_filename();

#endif // AUDIO_H
