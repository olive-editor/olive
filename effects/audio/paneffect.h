#ifndef PANEFFECT_H
#define PANEFFECT_H

#include "../effect.h"

class PanEffect : public Effect {
public:
    PanEffect(Clip* c);
	void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);

	EffectField* pan_val;
};

#endif // PANEFFECT_H
