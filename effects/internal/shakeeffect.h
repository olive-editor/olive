#ifndef SHAKEEFFECT_H
#define SHAKEEFFECT_H

#include "project/effect.h"

#define RANDOM_VAL_SIZE 30

class ShakeEffect : public Effect {
	Q_OBJECT
public:
	ShakeEffect(Clip* c, const EffectMeta* em);
    void process_coords(double timecode, GLTextureCoords& coords, int data);

	EffectField* intensity_val;
	EffectField* rotation_val;
	EffectField* frequency_val;
private:
	double random_vals[RANDOM_VAL_SIZE];
};

#endif // SHAKEEFFECT_H
