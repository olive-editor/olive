#ifndef FREI0REFFECT_H
#define FREI0REFFECT_H

#include "project/effect.h"

#include <frei0r.h>

#include "io/crossplatformlib.h"

typedef void (*f0rGetParamInfo)(f0r_param_info_t * info,
								int param_index );

class Frei0rEffect : public Effect {
	Q_OBJECT
public:
	Frei0rEffect(Clip* c, const EffectMeta* em);
	~Frei0rEffect();

	virtual void process_image(double timecode, uint8_t* input, uint8_t* output, int size);
private:
    ModulePtr handle;
	f0r_instance_t instance;
	int param_count;
	f0rGetParamInfo get_param_info;
};

#endif // FREI0REFFECT_H
