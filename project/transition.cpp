#include "transition.h"

#include "mainwindow.h"
#include "clip.h"
#include "sequence.h"
#include "debug.h"

#include "effects/internal/crossdissolvetransition.h"
#include "effects/internal/linearfadetransition.h"
#include "effects/internal/exponentialfadetransition.h"
#include "effects/internal/logarithmicfadetransition.h"
#include "effects/internal/cubetransition.h"

#include "ui/labelslider.h"

#include "panels/panels.h"
#include "panels/timeline.h"

#include <QMessageBox>

Transition::Transition(Clip* c, Clip* s, const EffectMeta* em) :
    Effect(c, em), secondary_clip(s),
    length(30)
{
    length_field = add_row("Length:", false)->add_field(EFFECT_FIELD_DOUBLE, "length");
    connect(length_field, SIGNAL(changed()), this, SLOT(set_length_from_slider()));
    length_field->set_double_default_value(30);
    length_field->set_double_minimum_value(0);

    LabelSlider* length_ui_ele = static_cast<LabelSlider*>(length_field->ui_element);
    length_ui_ele->set_display_type(LABELSLIDER_FRAMENUMBER);
    length_ui_ele->set_frame_rate(parent_clip->sequence->frame_rate);
}

int Transition::copy(Clip *c, Clip* s) {
    int copy_index = create_transition(c, s, meta);
    c->sequence->transitions.at(copy_index)->length = length;
    return copy_index;
}

void Transition::set_length(long l) {
    length = l;
    length_field->set_double_value(l);
}

long Transition::get_true_length() {
    return length;
}

long Transition::get_length() {
    if (secondary_clip != NULL) {
        return length * 2;
    }
    return length;
}

void Transition::set_length_from_slider() {
    set_length(length_field->get_double_value(0));
    update_ui(false);
}

Transition* get_transition_from_meta(Clip* c, Clip* s, const EffectMeta* em) {
    if (!em->filename.isEmpty()) {
        // load effect from file
        return new Transition(c, s, em);
    } else if (em->internal >= 0 && em->internal < TRANSITION_INTERNAL_COUNT) {
        // must be an internal effect
        switch (em->internal) {
        case TRANSITION_INTERNAL_CROSSDISSOLVE: return new CrossDissolveTransition(c, s, em);
        case TRANSITION_INTERNAL_LINEARFADE: return new LinearFadeTransition(c, s, em);
        case TRANSITION_INTERNAL_EXPONENTIALFADE: return new ExponentialFadeTransition(c, s, em);
        case TRANSITION_INTERNAL_LOGARITHMICFADE: return new LogarithmicFadeTransition(c, s, em);
        case TRANSITION_INTERNAL_CUBE: return new CubeTransition(c, s, em);
        }
    } else {
        dout << "[ERROR] Invalid transition data";
        QMessageBox::critical(mainWindow, "Invalid transition", "No candidate for transition '" + em->name + "'. This transition may be corrupt. Try reinstalling it or Olive.");
    }
    return NULL;
}

int create_transition(Clip* c, Clip* s, const EffectMeta* em) {
    Transition* t = get_transition_from_meta(c, s, em);
    if (t != NULL) {
        c->sequence->transitions.append(t);
        return c->sequence->transitions.size() - 1;
    }
    return -1;
}
