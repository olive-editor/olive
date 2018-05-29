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

Clip::Clip(const Clip &c) {
	init();
	copy(c);
}

Clip& Clip::operator= (const Clip& c) {
	init();
	copy(c);
	return *this;
}

void Clip::copy(const Clip& c) {
	name = c.name;
	clip_in = c.clip_in;
	timeline_in = c.timeline_in;
	timeline_out = c.timeline_out;
	track = c.track;
	color_r = c.color_r;
	color_g = c.color_g;
	color_b = c.color_b;
	sequence = c.sequence;
	media = c.media;
	media_stream = c.media_stream;
}

void Clip::init() {
	reset();
	clip_in = timeline_in = timeline_out = track = undeletable = 0;
	texture = NULL;
	pkt = new AVPacket();
}

void Clip::reset() {
	cache_size = cache_A.offset = cache_B.offset = open = pkt_written = cache_A.written = cache_B.written = cache_A.unread = cache_B.unread = reached_end = reset_audio = frame_sample_index = 0;
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
