#ifndef AUDIONOISEEFFECT_H
#define AUDIONOISEEFFECT_H

#include "../effect.h"

class AudioNoiseEffect : public Effect {
public:
	AudioNoiseEffect(Clip* c);
	void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);

	EffectField* amount_val;
	EffectField* mix_val;
};

#endif // AUDIONOISEEFFECT_H
