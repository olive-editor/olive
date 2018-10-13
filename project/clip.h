#ifndef CLIP_H
#define CLIP_H

#include <QWaitCondition>
#include <QMutex>
#include <QVector>

#define SKIP_TYPE_DISCARD 0
#define SKIP_TYPE_SEEK 1

class Cacher;
class Effect;
class Transition;
class QOpenGLFramebufferObject;
class ComboAction;
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
struct AVFilterGraph;
struct AVFilterContext;
class QOpenGLTexture;

struct ClipCache {
	AVFrame** frames;
	long offset;
	bool written;
	bool unread;
	int write_count;
	QMutex mutex;
};

/*struct ClipPlayback {

};*/

struct Clip
{
    Clip(Sequence* s);
	~Clip();
    Clip* copy(Sequence* s);
    void reset_audio();
	void reset();
	void refresh();
	void clear_queue();

	// timeline variables
    Sequence* sequence;
    bool enabled;
    long clip_in;
    long timeline_in;
    long timeline_out;
    int track;
    bool undeletable;
    int load_id;
	QString name;
    long get_timeline_in_with_transition();
    long get_timeline_out_with_transition();
    quint8 color_r;
    quint8 color_g;
    quint8 color_b;
    long getLength();
	long getMaximumLength();
	void recalculateMaxLength();
	double getMediaFrameRate();
    void* media; // attached media
    int media_type;
    int media_stream;
	int getWidth();
	int getHeight();
	double speed;
	bool reverse;
	long calculated_length;
	int skip_type;
	void refactor_frame_rate(ComboAction* ca, double multiplier, bool change_timeline_points);

	// other variables (should be "duplicated" in copy())
    QList<Effect*> effects;
    QVector<int> linked;
    Transition* opening_transition;
    Transition* closing_transition;

    // media handling
    AVFormatContext* formatCtx;
    AVStream* stream;
    AVCodec* codec;
    AVCodecContext* codecCtx;
    AVPacket* pkt;
	AVFrame* frame;

	bool reached_end; // deprecated
	bool pkt_written;
    bool open;
    bool finished_opening;
	bool replaced;

	int64_t rev_target;

	// caching functions
	bool use_existing_frame;
    bool multithreaded;
    Cacher* cacher;
	int cache_size; // deprecated
    QWaitCondition can_cache;
	int max_queue_size;
	QVector<AVFrame*> queue;
	ClipCache cache_A; // deprecated
	QMutex queue_lock;
    QMutex lock;
	QMutex open_lock;

	// converters/filters
	AVFilterGraph* filter_graph;
	AVFilterContext* buffersink_ctx;
	AVFilterContext* buffersrc_ctx;

	// video playback variables
	QOpenGLFramebufferObject** fbo;
    QOpenGLTexture* texture;
    long texture_frame;
	bool autoscale;

	// audio playback variables
	bool maintain_audio_pitch;
    int frame_sample_index;
    int audio_buffer_write;
    bool audio_reset;
    bool audio_just_reset;
	long audio_target_frame;
};

#endif // CLIP_H
