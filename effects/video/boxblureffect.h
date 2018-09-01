#ifndef BOXBLUREFFECT_H
#define BOXBLUREFFECT_H

#include "../effect.h"

class BoxBlurEffect : public Effect
{
public:
    BoxBlurEffect(Clip* c);

	void process_gl(double timecode, GLTextureCoords &coords);
private:
    EffectField* radius_val;
    EffectField* iteration_val;
    EffectField* horiz_val;
    EffectField* vert_val;
};

#endif // BOXBLUREFFECT_H
