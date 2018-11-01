#ifndef SHAKEEFFECT_H
#define SHAKEEFFECT_H

#include "../effect.h"

#define RANDOM_VAL_SIZE 30

class ShakeEffect : public Effect {
	Q_OBJECT
public:
	ShakeEffect(Clip* c, const EffectMeta* em);
	void process_coords(double timecode, GLTextureCoords& coords);

	EffectField* intensity_val;
	EffectField* rotation_val;
	EffectField* frequency_val;
private:
	int evolution;

	double random_vals[RANDOM_VAL_SIZE];

	/*double multiplier_1;
	double multiplier_2;
	double multiplier_3;
	double multiplier_4;
	double multiplier_5;
	double multiplier_6;

	double adder_1;
	double adder_2;
	double adder_3;
	double adder_4;
	double adder_5;
	double adder_6;*/
};

#endif // SHAKEEFFECT_H
