#include "sequence.h"

#include "project/clip.h"
#include "effects/transition.h"

#include <QDebug>

Sequence::Sequence() {}

Sequence::~Sequence() {
    // dealloc all clips
    for (int i=0;i<clips.size();i++) {
        delete clips.at(i);
    }
}

Sequence* Sequence::copy() {
    Sequence* s = new Sequence();
    s->name = name + " (copy)";
    s->width = width;
    s->height = height;
    s->frame_rate = frame_rate;
    s->audio_frequency = audio_frequency;
    s->audio_layout = audio_layout;
    for (int i=0;i<clip_count();i++) {
        Clip* c = get_clip(i);
        if (c != NULL) {
            Clip* copy = c->copy();
            copy->linked = c->linked;
            s->add_clip(copy);
        }
    }
    return s;
}

int Sequence::add_clip(Clip* c) {
    clips.append(c);
    return clip_count() - 1;
}

int Sequence::clip_count() {
    return clips.size();
}

Clip* Sequence::get_clip(int i) {
    return clips.at(i);
}

void Sequence::replace_clip(int i, Clip* c) {
    clips[i] = c;
}

long Sequence::getEndFrame() {
    long end = 0;
    for (int j=0;j<clip_count();j++) {
        Clip* c = get_clip(j);
        if (c != NULL && c->timeline_out > end) {
            end = c->timeline_out;
        }
    }
    return end;
}

void Sequence::destroy_clip(int i, bool del) {
    if (del) delete clips.at(i);
    clips.removeAt(i);
}

void Sequence::get_track_limits(int* video_tracks, int* audio_tracks) {
	int vt = 0;
	int at = 0;
	for (int j=0;j<clip_count();j++) {
        Clip* c = get_clip(j);
        if (c != NULL) {
            if (c->track < 0 && c->track < vt) { // video clip
                vt = c->track;
            } else if (c->track > at) {
                at = c->track;
            }
        }
	}
	if (video_tracks != NULL) *video_tracks = vt;
	if (audio_tracks != NULL) *audio_tracks = at;
}

// static variable for the currently active sequence
Sequence* sequence = NULL;
