#include "media.h"

#include <QDebug>

extern "C" {
	#include <libavformat/avformat.h>
}

#include "project/clip.h"

Media::Media() : ready(false) {}

Media::~Media() {
    reset();
}

void Media::reset() {
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

MediaStream* Media::get_stream_from_file_index(int index) {
    for (int i=0;i<video_tracks.size();i++) {
        if (video_tracks.at(i)->file_index == index) {
            return video_tracks.at(i);
        }
    }
    for (int i=0;i<audio_tracks.size();i++) {
        if (audio_tracks.at(i)->file_index == index) {
            return audio_tracks.at(i);
        }
    }
    return NULL;
}

int guess_layout_from_channels(int channel_count) {
    if (channel_count == 1) {
        return AV_CH_LAYOUT_MONO;
    } else {
        qDebug() << "[WARNING] Could not detect audio channel layout - assuming stereo";
        return AV_CH_LAYOUT_STEREO;
    }
}
