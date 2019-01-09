/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
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
