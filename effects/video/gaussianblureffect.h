#ifndef GAUSSIANBLUREFFECT_H
#define GAUSSIANBLUREFFECT_H

#include "../effect.h"

class GaussianBlurEffect : public Effect
{
public:
	GaussianBlurEffect(Clip* c);
	void process_image(double timecode, uint8_t *data, int width, int height);
	void process_gl(double timecode, GLTextureCoords &coords);
	void clean_gl();
private:
	QOpenGLShaderProgram program;
	QOpenGLShader vert;
	QOpenGLShader frag;

	EffectField* amount_val;
	EffectField* horiz_val;
	EffectField* vert_val;

	bool bound;
};

#endif // GAUSSIANBLUREFFECT_H
