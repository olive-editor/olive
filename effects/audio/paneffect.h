#ifndef PANEFFECT_H
#define PANEFFECT_H

#include "../effect.h"

class PanEffect : public Effect {
public:
	PanEffect(Clip* c);
	void refresh();
	void process_audio(quint8* samples, int nb_bytes);
	Effect* copy(Clip* c);

	EffectField* pan_val;
};

#endif // PANEFFECT_H
