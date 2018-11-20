#ifndef LOGARITHMICFADETRANSITION_H
#define LOGARITHMICFADETRANSITION_H

#include "project/transition.h"

class LogarithmicFadeTransition : public Transition {
public:
	LogarithmicFadeTransition(Clip* c, const EffectMeta* em);
	void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);
};

#endif // LOGARITHMICFADETRANSITION_H
