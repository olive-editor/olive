#ifndef LINEARFADETRANSITION_H
#define LINEARFADETRANSITION_H

#include "../effect.h"

class LinearFadeTransition : public Effect {
public:
	LinearFadeTransition(Clip* c, const EffectMeta* em);
	void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);
};

#endif // LINEARFADETRANSITION_H
