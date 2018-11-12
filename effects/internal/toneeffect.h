#ifndef TONEEFFECT_H
#define TONEEFFECT_H

#include "project/effect.h"

class ToneEffect : public Effect {
public:
	ToneEffect(Clip *c, const EffectMeta* em);
	void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);

	EffectField* type_val;
	EffectField* freq_val;
	EffectField* amount_val;
	EffectField* mix_val;
private:
	int sinX;
};

#endif // TONEEFFECT_H
