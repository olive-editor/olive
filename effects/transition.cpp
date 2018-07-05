#include "transition.h"

Transition::Transition() {
    length = 30;
    link = NULL;
}

Transition* Transition::copy() {
    return new Transition();
}

void Transition::process_transition(float) {}
