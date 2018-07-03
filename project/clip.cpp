#include "clip.h"

#include "project/effect.h"
#include "io/media.h"
#include "playback/playback.h"
#include "playback/cacher.h"

#include <QDebug>

extern "C" {
	#include <libavformat/avformat.h>
}

Clip::Clip() {
    reset();
    enabled = true;
    clip_in = 0;
    timeline_in = 0;
    timeline_out = 0;
    track = 0;
    undeletable = 0;
    texture = NULL;
    opening_transition = NULL;
    closing_transition = NULL;
    pkt = new AVPacket();
}

Clip* Clip::copy() {
    Clip* copy = new Clip();

    copy->enabled = enabled;
    copy->id = id;
    copy->name = QString(name);
    copy->clip_in = clip_in;
    copy->timeline_in = timeline_in;
    copy->timeline_out = timeline_out;
    copy->track = track;
    copy->color_r = color_r;
    copy->color_g = color_g;
    copy->color_b = color_b;
    copy->sequence = sequence;
    copy->media = media;
    copy->media_stream = media_stream;

    for (int i=0;i<effects.size();i++) {
        copy->effects.append(effects.at(i)->copy(copy));
    }

    return copy;
}

void Clip::reset() {
    cache_size = false;
    audio_just_reset = false;
    cache_A.offset = false;
    cache_B.offset = false;
    open = false;
    finished_opening = false;
    pkt_written = false;
    cache_A.written = false;
    cache_B.written = false;
    cache_A.unread = false;
    cache_B.unread = false;
    reached_end = false;
    reset_audio = false;
    frame_sample_index = false;
    audio_buffer_write = false;
    need_new_audio_frame = false;
	texture_frame = -1;
	formatCtx = NULL;
	stream = NULL;
	codec = NULL;
	codecCtx = NULL;
	texture = NULL;
	cache_A.frames = NULL;
	cache_B.frames = NULL;
}

Clip::~Clip() {
    if (open) {
		close_clip(this);
	}

    // make sure clip has closed before clip is destroyed
    open_lock.lock();
	open_lock.unlock();

    if (opening_transition != NULL) delete opening_transition;
    if (closing_transition != NULL) delete closing_transition;

    for (int i=0;i<effects.size();i++) {
        delete effects.at(i);
    }
	av_packet_unref(pkt);
	delete pkt;
}

// timeline functions
long Clip::getLength() {
	return timeline_out - timeline_in;
}
