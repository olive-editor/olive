#ifndef EXPONENTIALFADETRANSITION_H
#define EXPONENTIALFADETRANSITION_H

#include "../effect.h"

class ExponentialFadeTransition : public Effect {
public:
	ExponentialFadeTransition(Clip* c, const EffectMeta* em);
	void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);
};

#endif // LINEARFADETRANSITION_H
