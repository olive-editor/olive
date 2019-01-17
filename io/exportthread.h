#ifndef EXPORTTHREAD_H
#define EXPORTTHREAD_H

#include <QThread>
#include <QOffscreenSurface>
#include <QMutex>
#include <QWaitCondition>

class ExportDialog;
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVStream;
struct AVCodec;
struct SwsContext;
struct SwrContext;

extern "C" {
	#include <libavcodec/avcodec.h>
}

#define COMPRESSION_TYPE_CBR 0
#define COMPRESSION_TYPE_CFR 1
#define COMPRESSION_TYPE_TARGETSIZE 2
#define COMPRESSION_TYPE_TARGETBR 3

class ExportThread : public QThread {
	Q_OBJECT
public:
	ExportThread();
	void run();

	// export parameters
	QString filename;
	bool video_enabled;
	int video_codec;
	int video_width;
	int video_height;
	double video_frame_rate;
	int video_compression_type;
	double video_bitrate;
	bool audio_enabled;
	int audio_codec;
	int audio_sampling_rate;
	int audio_bitrate;
	long start_frame;
	long end_frame;

	QOffscreenSurface surface;

	ExportDialog* ed;

	bool continueEncode;
signals:
	void progress_changed(int value, qint64 remaining_ms);
public slots:
	void wake();
private:
	bool encode(AVFormatContext* ofmt_ctx, AVCodecContext* codec_ctx, AVFrame* frame, AVPacket* packet, AVStream* stream, bool rescale);
	bool setupVideo();
	bool setupAudio();
	bool setupContainer();

	AVFormatContext* fmt_ctx;
	AVStream* video_stream;
	AVCodec* vcodec;
	AVCodecContext* vcodec_ctx;
	AVFrame* video_frame;
	AVFrame* sws_frame;
	SwsContext* sws_ctx;
	AVStream* audio_stream;
	AVCodec* acodec;
	AVFrame* audio_frame;
	AVFrame* swr_frame;
	AVCodecContext* acodec_ctx;
	AVPacket video_pkt;
	AVPacket audio_pkt;
	SwrContext* swr_ctx;

	bool vpkt_alloc;
	bool apkt_alloc;

	int aframe_bytes;
	int ret;
	char* c_filename;

	QMutex mutex;
	QWaitCondition waitCond;
};

#endif // EXPORTTHREAD_H
