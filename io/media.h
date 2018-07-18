#ifndef MEDIA_H
#define MEDIA_H

#include <QString>
#include <QVector>
#include <QMetaType>
#include <QVariant>
#include <QMutex>
#include <QPixmap>

struct Sequence;

struct MediaStream {
	int file_index;
	int video_width;
	int video_height;
	bool infinite_length;
    int audio_channels;

    // preview thumbnail/waveform
    bool preview_done;
    QImage video_preview; // TODO change to QPixmap
    QVector<qint8> audio_preview;
};

struct Media
{
    Media();
    ~Media();

	QString url;
    QString name;
	int64_t length;
    QVector<MediaStream*> video_tracks;
    QVector<MediaStream*> audio_tracks;
    int save_id;
    bool ready;
	long get_length_in_frames(float frame_rate);
    MediaStream* get_stream_from_file_index(int index);
};

int guess_layout_from_channels(int channel_count);

#endif // MEDIA_H
