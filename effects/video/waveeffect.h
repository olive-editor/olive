#ifndef WAVEEFFECT_H
#define WAVEEFFECT_H

#include "../effect.h"

class WaveEffect : public Effect {
	Q_OBJECT
public:
	WaveEffect(Clip* c);
	void process_shader(double timecode);
private:
	EffectField* frequency_val;
	EffectField* intensity_val;
	EffectField* evolution_val;

	EffectField* vertical_val;
};

#endif // WAVEEFFECT_H
