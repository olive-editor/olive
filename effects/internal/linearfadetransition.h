#ifndef LINEARFADETRANSITION_H
#define LINEARFADETRANSITION_H

#include "project/transition.h"

class LinearFadeTransition : public Transition {
public:
	LinearFadeTransition(Clip* c, const EffectMeta* em);
	void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);
};

#endif // LINEARFADETRANSITION_H
