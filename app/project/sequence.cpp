/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "sequence.h"

#include "clip.h"
#include "transition.h"

#include <QCoreApplication>

#include "debug.h"

Sequence::Sequence() :
	playhead(0),
	using_workarea(false),
	workarea_in(0),
	workarea_out(0),
	wrapper_sequence(false)
{
}

Sequence::~Sequence() {
	// dealloc all clips
	for (int i=0;i<clips.size();i++) {
		delete clips.at(i);
	}
}

Sequence* Sequence::copy() {
	Sequence* s = new Sequence();
	s->name = QCoreApplication::translate("Sequence", "%1 (copy)").arg(name);
	s->width = width;
	s->height = height;
	s->frame_rate = frame_rate;
	s->audio_frequency = audio_frequency;
	s->audio_layout = audio_layout;
	s->clips.resize(clips.size());
	for (int i=0;i<clips.size();i++) {
		Clip* c = clips.at(i);
		if (c == nullptr) {
			s->clips[i] = nullptr;
		} else {
			Clip* copy = c->copy(s);
			copy->linked = c->linked;
			s->clips[i] = copy;
		}
	}
	return s;
}

long Sequence::getEndFrame() {
	long end = 0;
	for (int j=0;j<clips.size();j++) {
		Clip* c = clips.at(j);
		if (c != nullptr && c->timeline_out > end) {
			end = c->timeline_out;
		}
	}
	return end;
}

void Sequence::hard_delete_transition(Clip *c, int type) {
	int transition_index = (type == TA_OPENING_TRANSITION) ? c->opening_transition : c->closing_transition;
	if (transition_index > -1) {
		bool del = true;

		Transition* t = transitions.at(transition_index);
		if (t->secondary_clip != nullptr) {
			for (int i=0;i<clips.size();i++) {
				Clip* comp = clips.at(i);
				if (comp != nullptr
						&& c != comp
						&& (c->opening_transition == transition_index
						|| c->closing_transition == transition_index)) {
					if (type == TA_OPENING_TRANSITION) {
						// convert to closing transition
						t->parent_clip = t->secondary_clip;
					}

					del = false;
					t->secondary_clip = nullptr;
				}
			}
		}

		if (del) {
			delete transitions.at(transition_index);
			transitions[transition_index] = nullptr;
		}

		if (type == TA_OPENING_TRANSITION) {
			c->opening_transition = -1;
		} else {
			c->closing_transition = -1;
		}
	}
}

void Sequence::getTrackLimits(int* video_tracks, int* audio_tracks) {
	int vt = 0;
	int at = 0;
	for (int j=0;j<clips.size();j++) {
		Clip* c = clips.at(j);
		if (c != nullptr) {
			if (c->track < 0 && c->track < vt) { // video clip
				vt = c->track;
			} else if (c->track > at) {
				at = c->track;
			}
		}
	}
	if (video_tracks != nullptr) *video_tracks = vt;
	if (audio_tracks != nullptr) *audio_tracks = at;
}

// static variable for the currently active sequence
Sequence* Olive::ActiveSequence = nullptr;
