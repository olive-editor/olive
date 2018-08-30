#ifndef INVERTEFFECT_H
#define INVERTEFFECT_H

#include "../effect.h"

class InvertEffect : public Effect {
	Q_OBJECT
public:
	InvertEffect(Clip* c);
	void process_gl(double timecode, GLTextureCoords& coords);
	void clean_gl();
private:
	EffectField* amount_val;
	QOpenGLShader frag;
	QOpenGLShader vert;
	QOpenGLShaderProgram program;
};

#endif // INVERTEFFECT_H
