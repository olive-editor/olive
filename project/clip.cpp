#include "clip.h"

#include "effects/effect.h"
#include "effects/transition.h"
#include "io/media.h"
#include "playback/playback.h"
#include "playback/cacher.h"
#include "panels/project.h"
#include "project/sequence.h"
#include "panels/timeline.h"

#include <QDebug>

extern "C" {
	#include <libavformat/avformat.h>
}

Clip::Clip(Sequence* s) :
    sequence(s),
    enabled(true),
    clip_in(0),
    timeline_in(0),
    timeline_out(0),
    track(0),
    undeletable(0),
    media(NULL),
    opening_transition(NULL),
    closing_transition(NULL),
	pkt(new AVPacket()),
	replaced(false),
	texture(NULL),
	fbo(NULL)
{
    reset();
}

Clip* Clip::copy(Sequence* s) {
    Clip* copy = new Clip(s);

    copy->enabled = enabled;
    copy->name = QString(name);
    copy->clip_in = clip_in;
    copy->timeline_in = timeline_in;
    copy->timeline_out = timeline_out;
    copy->track = track;
    copy->color_r = color_r;
    copy->color_g = color_g;
    copy->color_b = color_b;
    copy->sequence = s;
    copy->media = media;
    copy->media_type = media_type;
    copy->media_stream = media_stream;

    for (int i=0;i<effects.size();i++) {
        copy->effects.append(effects.at(i)->copy(copy));
    }

    if (opening_transition != NULL) copy->opening_transition = opening_transition->copy();
    if (closing_transition != NULL) copy->closing_transition = closing_transition->copy();

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
    audio_reset = false;
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

void Clip::reset_audio() {
    switch (media_type) {
    case MEDIA_TYPE_FOOTAGE:
        audio_reset = true;
        frame_sample_index = 0;
        audio_buffer_write = 0;
		reached_end = false;
        break;
    case MEDIA_TYPE_SEQUENCE:
    {
        Sequence* nested_sequence = static_cast<Sequence*>(media);
        for (int i=0;i<nested_sequence->clip_count();i++) {
            Clip* c = nested_sequence->get_clip(i);
            if (c != NULL) c->reset_audio();
        }
    }
        break;
    }
}

void Clip::refresh() {
	// validates media if it was replaced
	if (replaced && media_type == MEDIA_TYPE_FOOTAGE) {
		Media* m = static_cast<Media*>(media);
		media_stream = (track < 0) ? m->video_tracks.at(0)->file_index : m->audio_tracks.at(0)->file_index;
	}
	replaced = false;

    // reinitializes all effects... just in case
    for (int i=0;i<effects.size();i++) {
		effects.at(i)->refresh();
    }
}

Clip::~Clip() {
    if (open) {
		close_clip(this);

        // make sure clip has closed before clip is destroyed
        if (multithreaded) {
            cacher->wait();
        }
    }

    if (opening_transition != NULL) delete opening_transition;
    if (closing_transition != NULL) delete closing_transition;

    for (int i=0;i<effects.size();i++) {
        delete effects.at(i);
    }
	av_packet_unref(pkt);
	delete pkt;
}

long Clip::get_timeline_in_with_transition() {
    if (opening_transition != NULL && opening_transition->link != NULL) {
        return timeline_in - opening_transition->link->length;
    } else {
        return timeline_in;
    }
}

long Clip::get_timeline_out_with_transition() {
    if (closing_transition != NULL && closing_transition->link != NULL) {
        return timeline_out + closing_transition->link->length;
    } else {
        return timeline_out;
    }
}

// timeline functions
long Clip::getLength() {
    return timeline_out - timeline_in;
}

long Clip::getMediaLength(double framerate) {
    switch (media_type) {
    case MEDIA_TYPE_FOOTAGE:
    {
        Media* m = static_cast<Media*>(media);
        if (m->get_stream_from_file_index(track < 0, media_stream)->infinite_length) {
            return LONG_MAX;
        } else {
            return m->get_length_in_frames(framerate);
        }
    }
        break;
    case MEDIA_TYPE_SEQUENCE:
    {
        Sequence* s = static_cast<Sequence*>(media);
        return refactor_frame_number(s->getEndFrame(), s->frame_rate, framerate);
    }
        break;
	case MEDIA_TYPE_SOLID:
		return timeline_out - timeline_in;
		break;
	}
	return 0;
}

int Clip::getWidth() {
	if (media == NULL && sequence != NULL) return sequence->width;
	switch (media_type) {
	case MEDIA_TYPE_FOOTAGE:
	{
		MediaStream* ms = static_cast<Media*>(media)->get_stream_from_file_index(track < 0, media_stream);
		if (ms != NULL) return ms->video_width;
        if (sequence != NULL) return sequence->width;
	}
	case MEDIA_TYPE_SEQUENCE:
	{
		Sequence* s = static_cast<Sequence*>(media);
		return s->width;
	}
	}
	return 0;
}

int Clip::getHeight() {
	if (media == NULL && sequence != NULL) return sequence->height;
	switch (media_type) {
	case MEDIA_TYPE_FOOTAGE:
	{
		MediaStream* ms = static_cast<Media*>(media)->get_stream_from_file_index(track < 0, media_stream);
		if (ms != NULL) return ms->video_height;
        if (sequence != NULL) return sequence->height;
	}
	case MEDIA_TYPE_SEQUENCE:
	{
		Sequence* s = static_cast<Sequence*>(media);
		return s->height;
	}
	}
	return 0;
}
