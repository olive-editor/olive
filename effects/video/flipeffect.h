#ifndef FLIPEFFECT_H
#define FLIPEFFECT_H

#include "../effect.h"

class FlipEffect : public Effect
{
public:
	FlipEffect(Clip* c);
	void process_gl(double timecode, QOpenGLShaderProgram &shaders, GLTextureCoords &coords);
private:
	EffectField* horizontal_field;
	EffectField* vertical_field;
};

#endif // FLIPEFFECT_H
