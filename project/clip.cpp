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
	init();
}

Clip* Clip::copy() {
    Clip* copy = new Clip();

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

void Clip::init() {
	reset();
	clip_in = timeline_in = timeline_out = track = undeletable = 0;
	texture = NULL;
	pkt = new AVPacket();
}

void Clip::reset() {
    cache_size = cache_A.offset = cache_B.offset = open = pkt_written = cache_A.written = cache_B.written = cache_A.unread = cache_B.unread = reached_end = reset_audio = frame_sample_index = audio_buffer_write = 0;
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
