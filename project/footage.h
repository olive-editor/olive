#ifndef FOOTAGE_H
#define FOOTAGE_H

#include <QString>
#include <QVector>
#include <QMetaType>
#include <QVariant>
#include <QMutex>
#include <QPixmap>
#include <QIcon>

#define VIDEO_PROGRESSIVE 0
#define VIDEO_TOP_FIELD_FIRST 1
#define VIDEO_BOTTOM_FIELD_FIRST 2

struct Sequence;
struct Clip;
class PreviewGenerator;
class MediaThrobber;

struct FootageStream {
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
	bool enabled;

	// preview thumbnail/waveform
	bool preview_done;
	QImage video_preview;
	QIcon video_preview_square;
	QVector<char> audio_preview;
	void make_square_thumb();
};

struct Footage {
	Footage();
	~Footage();

	QString url;
	QString name;
	int64_t length;
	QVector<FootageStream> video_tracks;
	QVector<FootageStream> audio_tracks;
	int save_id;
	bool ready;
	bool invalid;
	double speed;

	PreviewGenerator* preview_gen;
	QMutex ready_lock;

	bool using_inout;
	long in;
	long out;

	long get_length_in_frames(double frame_rate);
	FootageStream *get_stream_from_file_index(bool video, int index);
	void reset();
};

#endif // FOOTAGE_H
