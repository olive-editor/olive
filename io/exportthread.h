#ifndef EXPORTTHREAD_H
#define EXPORTTHREAD_H

#include <QThread>
#include <QOffscreenSurface>

class ExportDialog;
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVStream;

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
private:
	bool encode(AVFormatContext* ofmt_ctx, AVCodecContext* codec_ctx, AVFrame* frame, AVPacket* packet, AVStream* stream);
	bool setupVideo();
	bool setupAudio();
	bool setupContainer();
};

#endif // EXPORTTHREAD_H
