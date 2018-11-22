#ifndef EXPONENTIALFADETRANSITION_H
#define EXPONENTIALFADETRANSITION_H

#include "project/transition.h"

class ExponentialFadeTransition : public Transition {
public:
    ExponentialFadeTransition(Clip* c, Clip* s, const EffectMeta* em);
	void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);
};

#endif // LINEARFADETRANSITION_H
