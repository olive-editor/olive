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
#define TRANSITION_INTERNAL_COUNT 4

int create_transition(Clip* c, Clip* s, const EffectMeta* em);

class Transition : public Effect {
public:
    Transition(Clip* c, Clip* s, const EffectMeta* em);
    int copy(Clip* c, Clip* s);
    Clip* secondary_clip;
    long get_length();
};

#endif // TRANSITION_H
