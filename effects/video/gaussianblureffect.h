#ifndef GAUSSIANBLUREFFECT_H
#define GAUSSIANBLUREFFECT_H

#include "../effect.h"

class GaussianBlurEffect : public Effect
{
public:
    GaussianBlurEffect(Clip* c);
	void process_gl(double timecode, GLTextureCoords &coords);
	void clean_gl();
private:
	QOpenGLShaderProgram program;
	QOpenGLShader vert;
	QOpenGLShader frag;

    EffectField* kernelsz_val;
    EffectField* sigma_val;
	EffectField* horiz_val;
	EffectField* vert_val;

	bool bound;
};

#endif // GAUSSIANBLUREFFECT_H
