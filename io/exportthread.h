#ifndef EXPORTTHREAD_H
#define EXPORTTHREAD_H

#include <QThread>
#include <QOffscreenSurface>

class ExportDialog;

class ExportThread : public QThread {
	Q_OBJECT
public:
    void run();

	// export parameters
	QString filename;
	bool video_enabled;
	int video_codec;
	int video_width;
	int video_height;
	double video_frame_rate;
	double video_bitrate;
	bool audio_enabled;
	int audio_codec;
	int audio_sampling_rate;
	int audio_bitrate;

	QOffscreenSurface surface;

    ExportDialog* ed;

    bool fail;
signals:
    void progress_changed(int value);
};

#endif // EXPORTTHREAD_H
