#include "transition.h"

#include "project/clip.h"

#include <QDebug>

Transition::Transition(int i, QString n) : id(i), name(n) {
    length = 30;
    link = NULL;
}

Transition::~Transition() {}

Transition* Transition::copy() {
	return NULL;
}

void Transition::process_transition(float) {}

Transition* create_transition(int transition_id, Clip* c) {
    if (c->track < 0) {
        switch (transition_id) {
        case VIDEO_DISSOLVE_TRANSITION: return new CrossDissolveTransition();
        }
    } else {
        switch (transition_id) {
		case AUDIO_LINEAR_FADE_TRANSITION: return new CrossDissolveTransition();
        }
    }
    qDebug() << "[ERROR] Invalid transition ID";
    return NULL;
}
