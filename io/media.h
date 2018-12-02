#ifndef MEDIA_H
#define MEDIA_H

#include <QString>
#include <QVector>
#include <QMetaType>
#include <QVariant>
#include <QMutex>
#include <QPixmap>

#define MEDIA_TYPE_FOOTAGE 0
#define MEDIA_TYPE_SEQUENCE 1
#define MEDIA_TYPE_FOLDER 2
#define MEDIA_TYPE_SOLID 3
#define MEDIA_TYPE_TONE 4

#define VIDEO_PROGRESSIVE 0
#define VIDEO_TOP_FIELD_FIRST 1
#define VIDEO_BOTTOM_FIELD_FIRST 2

struct Sequence;
struct Clip;
class PreviewGenerator;
class MediaThrobber;

struct MediaStream {
	int file_index;
	int video_width;
	int video_height;
	bool infinite_length;
    double video_frame_rate;
    int video_interlacing;
	int video_auto_interlacing;
    int audio_channels;
    int audio_layout;
    int audio_frequency;

    // preview thumbnail/waveform
    bool preview_done;
    QImage video_preview;
	QVector<char> audio_preview;
};

struct Media {
    Media();
	~Media();

	QString url;
    QString name;
	int64_t length;
    QVector<MediaStream*> video_tracks;
    QVector<MediaStream*> audio_tracks;
    int save_id;
    bool ready;
    bool invalid;

    PreviewGenerator* preview_gen;
    QMutex ready_lock;

    bool using_inout;
    long in;
    long out;

    long get_length_in_frames(double frame_rate);
    MediaStream* get_stream_from_file_index(bool video, int index);
    void reset();
};

#endif // MEDIA_H
