#include "transition.h"

#include "mainwindow.h"
#include "clip.h"
#include "sequence.h"
#include "debug.h"

#include "effects/internal/crossdissolvetransition.h"
#include "effects/internal/linearfadetransition.h"
#include "effects/internal/exponentialfadetransition.h"
#include "effects/internal/logarithmicfadetransition.h"

#include <QMessageBox>

Transition::Transition(Clip* c, const EffectMeta* em) : Effect(c, em) {
    add_row("Length:")->add_field(EFFECT_FIELD_DOUBLE);
}

int Transition::copy(Clip *c) {
    int copy_index = create_transition(c, meta);
    c->sequence->transitions.at(copy_index)->length = length;
    return copy_index;
}

Transition* get_transition_from_meta(Clip* c, const EffectMeta* em) {
    if (!em->filename.isEmpty()) {
        // load effect from file
        return new Transition(c, em);
    } else if (em->internal >= 0 && em->internal < TRANSITION_INTERNAL_COUNT) {
        // must be an internal effect
        switch (em->internal) {
        case TRANSITION_INTERNAL_CROSSDISSOLVE: return new CrossDissolveTransition(c, em);
        case TRANSITION_INTERNAL_LINEARFADE: return new LinearFadeTransition(c, em);
        case TRANSITION_INTERNAL_EXPONENTIALFADE: return new ExponentialFadeTransition(c, em);
        case TRANSITION_INTERNAL_LOGARITHMICFADE: return new LogarithmicFadeTransition(c, em);
        }
    } else {
        dout << "[ERROR] Invalid transition data";
        QMessageBox::critical(mainWindow, "Invalid transition", "No candidate for transition '" + em->name + "'. This transition may be corrupt. Try reinstalling it or Olive.");
    }
    return NULL;
}

int create_transition(Clip* c, const EffectMeta* em) {
    Transition* t = get_transition_from_meta(c, em);
    if (t != NULL) {
        c->sequence->transitions.append(t);
        return c->sequence->transitions.size() - 1;
    }
    return -1;
}
