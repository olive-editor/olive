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
	Clip(const Clip &c);
	Clip& operator= (const Clip&); // explicitly defaulted copy assignment
	~Clip();
	void copy(const Clip& c);
	void init();
	void reset();
	bool undeletable;

	// timeline variables
	QString name;
	long clip_in;
	long timeline_in;
	long timeline_out;
	int track;
	uint8_t color_r;
	uint8_t color_g;
	uint8_t color_b;
	long getLength();

	// inherited information (should be copied in copy())
	Media* media; // attached media
	MediaStream* media_stream;
	Sequence* sequence;

	// other variables (should be "duplicated" in copy())
    QList<Effect*> effects;
//	QVector<Clip&> linkedClips;

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
	uint16_t cache_size;
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
	int frame_sample_index;
	bool reset_audio;
};

#endif // CLIP_H
