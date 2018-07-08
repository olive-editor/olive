#include "transition.h"

#include "project/clip.h"

#include <QDebug>

Transition::Transition() {
    length = 30;
    link = NULL;
}

Transition* Transition::copy() {
    return new Transition();
}

void Transition::process_transition(float) {}

Transition* create_transition(int transition_id, Clip* c) {
    if (c->track < 0) {
        switch (transition_id) {
        case VIDEO_DISSOLVE_TRANSITION: return new CrossDissolveTransition();
        }
    } else {
        switch (transition_id) {
        }
    }
    qDebug() << "[ERROR] Invalid transition ID";
    return NULL;
}
