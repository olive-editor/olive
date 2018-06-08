#include "sequence.h"

#include "project/clip.h"

#include <QDebug>

Sequence::Sequence() {
    undo_pointer = -1;
}

Sequence::~Sequence() {
}

Clip& Sequence::new_clip() {
	clips.append(Clip());
	return clips[clips.size() - 1];
}

Clip& Sequence::insert_clip(const Clip& c) {
	clips.append(c);
	return clips[clips.size() - 1];
}

int Sequence::clip_count() {
	return clips.count();
}

Clip& Sequence::get_clip(int i) {
	return clips[i];
}

void Sequence::delete_clip(int i) {
	// remove any potential link references to clip
//	for (size_t j=0;j<clip_count();j++) {
//		Clip* c = getClip(j);
//		for (size_t k=0;k<c->linkedClips.size();k++) {
//			if (c->linkedClips[k] == clips[i]) {
//				c->linkedClips.erase(c->linkedClips.begin()+k);
//			}
//		}
//	}

	// finally remove from vector
	clips.removeAt(i);
}

long Sequence::getEndFrame() {
	long end = 0;
	for (int j=0;j<clip_count();j++) {
		Clip& c = get_clip(j);
		if (c.timeline_out > end) {
			end = c.timeline_out;
		}
	}
	return end;
}

void Sequence::get_track_limits(int* video_tracks, int* audio_tracks) {
	int vt = 0;
	int at = 0;
	for (int j=0;j<clip_count();j++) {
		Clip& c = get_clip(j);
		if (c.track < 0 && c.track < vt) { // video clip
			vt = c.track;
		} else if (c.track > at) {
			at = c.track;
		}
	}
	if (video_tracks != NULL) *video_tracks = vt;
	if (audio_tracks != NULL) *audio_tracks = at;
}

void Sequence::delete_area(long in, long out, int track) {
	for (int i=0;i<clip_count();i++) {
		Clip& c = get_clip(i);
		if (c.track == track && !c.undeletable) {
			if (c.timeline_in >= in && c.timeline_out <= out) {
				// clips falls entirely within deletion area
				delete_clip(i);
				i--;
			} else if (c.timeline_in < in && c.timeline_out > out) {
				// middle of clip is within deletion area
				clips.append(clips.at(i)); // copy clip

				Clip& pre = get_clip(i);
				Clip& post = get_clip(clips.size()-1);

				pre.timeline_out = in;
				post.timeline_in = out;
				post.clip_in = pre.clip_in + pre.getLength() + (out - in);
			} else if (c.timeline_in < in && c.timeline_out > in) {
				// only out point is in deletion area
				c.timeline_out = in;
			} else if (c.timeline_in < out && c.timeline_out > out) {
				// only in point is in deletion area
				c.clip_in += out - c.timeline_in;
				c.timeline_in = out;
			}
		}
	}
}

void Sequence::split_clip(int i, long frame) {
	Clip& pre = get_clip(i);
	if (pre.timeline_in < frame && pre.timeline_out > frame) { // guard against attempts to split at in/out points
		clips.append(pre); // copy clip

		Clip& post = get_clip(clips.size()-1);

		pre.timeline_out = frame;
		post.timeline_in = frame;
		post.clip_in = pre.clip_in + pre.getLength();
	}
}

void Sequence::split_at_playhead(long frame) {
	for (int j=0;j<clip_count();j++) {
		Clip& c = get_clip(j);
		if (c.timeline_in < frame && c.timeline_out > frame) {
			split_clip(j, frame);
		}
	}
}

void Sequence::undo_add_current() {
    undo_pointer++;
    undo_stack.resize(undo_pointer);
    undo_stack.append(clips);
    qDebug() << "a pointer:" << undo_pointer << undo_stack.size();
}

void Sequence::undo() {
    if (undo_pointer > 0) {
        undo_pointer--;
        clips = undo_stack[undo_pointer];
    } else {
        qDebug() << "[INFO] No more undos";
    }
    qDebug() << "u pointer:" << undo_pointer << undo_stack.size();
}
void Sequence::redo() {
    if (undo_pointer == undo_stack.size() - 1) {
        qDebug() << "[INFO] No more redos";
    } else {
        undo_pointer++;
        clips = undo_stack[undo_pointer];
    }
    qDebug() << "r pointer:" << undo_pointer << undo_stack.size();
}
