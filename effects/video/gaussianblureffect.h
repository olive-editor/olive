#ifndef GAUSSIANBLUREFFECT_H
#define GAUSSIANBLUREFFECT_H

#include "../effect.h"

class GaussianBlurEffect : public Effect
{
public:
	GaussianBlurEffect(Clip* c);
	void process_gl(double timecode, QOpenGLShaderProgram &shaders, GLTextureCoords &coords);
};

#endif // GAUSSIANBLUREFFECT_H
