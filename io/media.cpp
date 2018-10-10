#include "media.h"

#include <QDebug>
#include "io/previewgenerator.h"

extern "C" {
	#include <libavformat/avformat.h>
}

#include "project/clip.h"

Media::Media() : ready(false), preview_gen(NULL) {}

Media::~Media() {
    reset();
}

void Media::reset() {
	if (preview_gen != NULL) {
		preview_gen->cancel();
		preview_gen->wait();
	}
    for (int i=0;i<video_tracks.size();i++) {
        delete video_tracks.at(i);
    }
    for (int i=0;i<audio_tracks.size();i++) {
        delete audio_tracks.at(i);
    }
    video_tracks.clear();
    audio_tracks.clear();
    ready = false;
}

long Media::get_length_in_frames(double frame_rate) {
    return ceil(((double) length / (double) AV_TIME_BASE) * frame_rate);
}

MediaStream* Media::get_stream_from_file_index(bool video, int index) {
    if (video) {
        for (int i=0;i<video_tracks.size();i++) {
            if (video_tracks.at(i)->file_index == index) {
                return video_tracks.at(i);
            }
        }
    } else {
        for (int i=0;i<audio_tracks.size();i++) {
            if (audio_tracks.at(i)->file_index == index) {
                return audio_tracks.at(i);
            }
        }
    }
    return NULL;
}
