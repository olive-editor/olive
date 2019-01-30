#include "footage.h"

#include <QDebug>
#include <QtMath>
#include <QPainter>
#include "io/previewgenerator.h"

extern "C" {
	#include <libavformat/avformat.h>
}

#include "project/clip.h"

Footage::Footage() :
	ready(false),
	preview_gen(nullptr),
	invalid(false),
	in(0),
	out(0),
	speed(1.0),
	alpha_is_premultiplied(false)
{
	ready_lock.lock();
}

Footage::~Footage() {
	reset();
}

void Footage::reset() {
	if (preview_gen != nullptr) {
		preview_gen->cancel();
		preview_gen->wait();
	}
	video_tracks.clear();
	audio_tracks.clear();
	ready = false;
}

long Footage::get_length_in_frames(double frame_rate) {
	if (length >= 0) return qFloor(((double) length / (double) AV_TIME_BASE) * frame_rate / speed);
	return 0;
}

FootageStream* Footage::get_stream_from_file_index(bool video, int index) {
	if (video) {
		for (int i=0;i<video_tracks.size();i++) {
			if (video_tracks.at(i).file_index == index) {
				return &video_tracks[i];
			}
		}
	} else {
		for (int i=0;i<audio_tracks.size();i++) {
			if (audio_tracks.at(i).file_index == index) {
				return &audio_tracks[i];
			}
		}
	}
	return nullptr;
}

void FootageStream::make_square_thumb() {
	// generate square version for QListView?
	int square_size = qMax(video_preview.width(), video_preview.height());
	QPixmap pixmap(square_size, square_size);
	pixmap.fill(Qt::transparent);
	QPainter p(&pixmap);
	int diff = (video_preview.width() - video_preview.height())>>1;
	int sqx = (diff < 0) ? -diff : 0;
	int sqy = (diff > 0) ? diff : 0;
	p.drawImage(sqx, sqy, video_preview);
	p.end();
	video_preview_square = QIcon(pixmap);
}
