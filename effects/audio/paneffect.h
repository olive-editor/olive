#ifndef PANEFFECT_H
#define PANEFFECT_H

#include "../effect.h"

class PanEffect : public Effect {
public:
    PanEffect(Clip* c);
    void process_audio(quint8* samples, int nb_bytes);

	EffectField* pan_val;
};

#endif // PANEFFECT_H
