#ifndef CHROMAKEYEFFECT_H
#define CHROMAKEYEFFECT_H

#include "effects/effect.h"

class ChromaKeyEffect : public Effect {
public:
	ChromaKeyEffect(Clip* c);
	void process_gl(double timecode, GLTextureCoords &coords);
	void clean_gl();
private:
	EffectField* color_field;
	EffectField* tolerance_field;
	QOpenGLShaderProgram program;
	QOpenGLShader vert;
	QOpenGLShader frag;
};

#endif // CHROMAKEYEFFECT_H
