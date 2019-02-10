#ifndef AUDIONOISEEFFECT_H
#define AUDIONOISEEFFECT_H

#include "project/effect.h"

class AudioNoiseEffect : public Effect {
    Q_OBJECT
public:
	AudioNoiseEffect(Clip* c, const EffectMeta* em);
	void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);

	EffectField* amount_val;
	EffectField* mix_val;
};

#endif // AUDIONOISEEFFECT_H
