#ifndef CROPEFFECT_H
#define CROPEFFECT_H

#include "../effect.h"

class CropEffect : public Effect
{
public:
	CropEffect(Clip* c);
	void process_gl(double timecode, GLTextureCoords &coords);
private:
	EffectField* left_field;
	EffectField* top_field;
	EffectField* right_field;
	EffectField* bottom_field;
};

#endif // CROPEFFECT_H
