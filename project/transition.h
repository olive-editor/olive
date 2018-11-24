#ifndef TRANSITION_H
#define TRANSITION_H

#include "effect.h"

#define TA_NO_TRANSITION 0
#define TA_OPENING_TRANSITION 1
#define TA_CLOSING_TRANSITION 2

#define TRANSITION_INTERNAL_CROSSDISSOLVE 0
#define TRANSITION_INTERNAL_LINEARFADE 1
#define TRANSITION_INTERNAL_EXPONENTIALFADE	2
#define TRANSITION_INTERNAL_LOGARITHMICFADE 3
#define TRANSITION_INTERNAL_CUBE 4
#define TRANSITION_INTERNAL_COUNT 5

int create_transition(Clip* c, Clip* s, const EffectMeta* em);

class Transition : public Effect {
    Q_OBJECT
public:
    Transition(Clip* c, Clip* s, const EffectMeta* em);
    int copy(Clip* c, Clip* s);
    Clip* secondary_clip;
    void set_length(long l);
    long get_true_length();
    long get_length();
private slots:
    void set_length_from_slider();
private:
    long length; // used only for transitions
    EffectField* length_field;
};

#endif // TRANSITION_H
