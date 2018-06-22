#ifndef CLIP_H
#define CLIP_H

#include <QWaitCondition>
#include <QMutex>
#include <QVector>

class Cacher;
class Effect;
struct Sequence;
struct Media;
struct MediaStream;

struct AVFormatContext;
struct AVStream;
struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct SwrContext;
class QOpenGLTexture;

struct ClipCache {
	AVFrame** frames;
	long offset;
	bool written;
	bool unread;
	QMutex mutex;
};

struct Clip
{
	Clip();
	~Clip();
    Clip* copy();
	void init();
	void reset();
	bool undeletable;

	// timeline variables
    bool enabled;
	QString name;
	long clip_in;
	long timeline_in;
	long timeline_out;
	int track;
    quint8 color_r;
    quint8 color_g;
    quint8 color_b;
	long getLength();

    // inherited information (should be set to the same references in copy())
	Media* media; // attached media
	MediaStream* media_stream;
	Sequence* sequence;

	// other variables (should be "duplicated" in copy())
    QList<Effect*> effects;
    QVector<Clip*> linked;

	// media handling
	AVFormatContext* formatCtx;
	AVStream* stream;
	AVCodec* codec;
	AVCodecContext* codecCtx;
	AVPacket* pkt;
	AVFrame* frame;
	bool pkt_written;
	bool reached_end;
	bool open;

	// caching functions
	bool multithreaded;
	Cacher* cacher;
	QWaitCondition can_cache;
    quint16 cache_size;
	ClipCache cache_A;
	ClipCache cache_B;
	QMutex lock;
	QMutex open_lock;

	// video playback variables
	SwsContext* sws_ctx;
	QOpenGLTexture* texture;
	long texture_frame;

	// audio playback variables
	SwrContext* swr_ctx;
    bool need_new_audio_frame;
	int frame_sample_index;
    int audio_buffer_write;
	bool reset_audio;
    bool audio_just_reset;
    long audio_target_frame;
};

#endif // CLIP_H
