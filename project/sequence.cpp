#include "sequence.h"

#include "project/clip.h"
#include "effects/transition.h"

#include <QDebug>

Sequence::Sequence() {
    undo_pointer = -1;
    undo_add_current();
}

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

void Sequence::delete_clip(int i) {
	// remove any potential link references to clip
    for (int j=0;j<clip_count();j++) {
        Clip* c = get_clip(j);
        if (c != NULL) {
            for (int k=0;k<c->linked.size();k++) {
                if (c->linked[k] == i) {
                    c->linked.removeAt(k);
                    break;
                }
            }
        }
    }

	// finally remove from vector
    delete clips.at(i);
    clips[i] = NULL;
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

//void Sequence::delete_area(long in, long out, int track) {

//}

int Sequence::split_clip(int p, long frame) {
    Clip* pre = get_clip(p);
    if (pre != NULL && pre->timeline_in < frame && pre->timeline_out > frame) { // guard against attempts to split at in/out points
        Clip* post = pre->copy();

        pre->timeline_out = frame;
        post->timeline_in = frame;
        post->clip_in = pre->clip_in + pre->getLength();

        long pre_length = pre->getLength();

        if (pre->closing_transition != NULL) {
            post->closing_transition = pre->closing_transition;
            pre->closing_transition = NULL;
            long post_length = post->getLength();
            if (post->closing_transition->length > post_length) {
                post->closing_transition->length = post_length;
            }
        }
        if (pre->opening_transition != NULL && pre->opening_transition->length > pre_length) {
            pre->opening_transition->length = pre_length;
        }

        return add_clip(post);
	}
    return -1;
}

void Sequence::undo_add_current() {
    if (undo_stack.size() == UNDO_LIMIT) {
        delete undo_stack.at(0);
        undo_stack.removeFirst();
    } else {
        undo_pointer++;
        while (undo_stack.size() > undo_pointer+1) {
            delete undo_stack.last();
            undo_stack.removeLast();
        }
    }

    // copy clips
    QVector<Clip*>* copied_clips = new QVector<Clip*>();
    for (int i=0;i<clip_count();i++) {
        Clip* original = get_clip(i);
        if (original != NULL) {
            Clip* copy = original->copy();
            copy->linked = original->linked;
            copied_clips->append(copy);
        }
    }
    undo_stack.append(copied_clips);
}

void Sequence::set_undo() {
    qDebug() << "[INFO] Setting undo pointer to" << undo_pointer;

    for (int i=0;i<clip_count();i++) {
        delete clips.at(i);
    }
    clips.clear();

    QVector<Clip*>* copy_from_list = undo_stack.at(undo_pointer);
    for (int i=0;i<copy_from_list->size();i++) {
        Clip* original = copy_from_list->at(i);
        Clip* copy = original->copy();
        copy->linked = original->linked;
        add_clip(copy);
    }
}

void Sequence::undo() {
    if (undo_pointer > 0) {
        undo_pointer--;
        set_undo();
    } else {
        qDebug() << "[INFO] No more undos";
    }
}
void Sequence::redo() {
    if (undo_pointer == undo_stack.size() - 1) {
        qDebug() << "[INFO] No more redos";
    } else {
        undo_pointer++;
        set_undo();
    }
}

// static variable for the currently active sequence
Sequence* sequence = NULL;
