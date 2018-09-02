#ifndef BOXBLUREFFECT_H
#define BOXBLUREFFECT_H

#include "../effect.h"

class BoxBlurEffect : public Effect
{
public:
    BoxBlurEffect(Clip* c);
	void process_shader(double timecode);
private:
    EffectField* radius_val;
    EffectField* iteration_val;
    EffectField* horiz_val;
    EffectField* vert_val;
};

#endif // BOXBLUREFFECT_H
