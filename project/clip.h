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

struct Clip
{
    Clip(Sequence* s);
	~Clip();
    Clip* copy(Sequence* s);
    void reset_audio();
	void reset();
	void refresh();
    long get_clip_in_with_transition();
	long get_timeline_in_with_transition();
	long get_timeline_out_with_transition();
	long getLength();
	long getMaximumLength();
	void recalculateMaxLength();
	double getMediaFrameRate();
	int getWidth();
	int getHeight();
	void refactor_frame_rate(ComboAction* ca, double multiplier, bool change_timeline_points);
	Sequence* sequence;

	// queue functions
	void queue_clear();
	void queue_remove_earliest();

	// timeline variables (should be copied in copy())
    bool enabled;
    long clip_in;
    long timeline_in;
    long timeline_out;
    int track;
	QString name;
    quint8 color_r;
    quint8 color_g;
    quint8 color_b;    
    void* media; // attached media
    int media_type;
    int media_stream;	
	double speed;
    double cached_fr;
	bool reverse;
	bool maintain_audio_pitch;
	bool autoscale;

	// other variables (should be deep copied/duplicated in copy())
    QList<Effect*> effects;
    QVector<int> linked;
    int opening_transition;
    Transition* get_opening_transition();
    int closing_transition;
    Transition* get_closing_transition();

	// media handling
    AVFormatContext* formatCtx;
    AVStream* stream;
    AVCodec* codec;
    AVCodecContext* codecCtx;
    AVPacket* pkt;
	AVFrame* frame;
	long calculated_length;

	// temporary variables
	int load_id;
	bool undeletable;
	bool reached_end;
	bool pkt_written;
    bool open;
    bool finished_opening;
    bool replaced;
	bool ignore_reverse;

	// caching functions
	bool use_existing_frame;
    bool multithreaded;
	Cacher* cacher;
    QWaitCondition can_cache;
	int max_queue_size;
	QVector<AVFrame*> queue;
	QMutex queue_lock;
    QMutex lock;
	QMutex open_lock;
    int64_t last_invalid_ts;

	// converters/filters
	AVFilterGraph* filter_graph;
	AVFilterContext* buffersink_ctx;
	AVFilterContext* buffersrc_ctx;

	// video playback variables
	QOpenGLFramebufferObject** fbo;
    QOpenGLTexture* texture;
	long texture_frame;

	// audio playback variables
	int64_t reverse_target;
    int frame_sample_index;
    int audio_buffer_write;
    bool audio_reset;
    bool audio_just_reset;
	long audio_target_frame;
};

#endif // CLIP_H
