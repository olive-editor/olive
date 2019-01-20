#ifndef FREI0REFFECT_H
#define FREI0REFFECT_H

#include "project/effect.h"

#include <frei0r/frei0r.h>

class Frei0rEffect : public Effect {
	Q_OBJECT
public:
	Frei0rEffect(Clip* c, const EffectMeta* em);
	~Frei0rEffect();

	virtual void process_image(double timecode, uint8_t* input, uint8_t* output, int size);
private:
	HMODULE modulePtr;
	f0r_instance_t instance;
};

#endif // FREI0REFFECT_H
