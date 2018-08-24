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
};

#endif // GAUSSIANBLUREFFECT_H
