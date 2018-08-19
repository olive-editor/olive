#ifndef INVERTEFFECT_H
#define INVERTEFFECT_H

#include "../effect.h"

//#include <QOpenGLShader>

class InvertEffect : public Effect {
	Q_OBJECT
public:
	InvertEffect(Clip* c);
	void process_gl(long p, QOpenGLShaderProgram& shader_prog, int *anchor_x, int *anchor_y);
	Effect* copy(Clip *c);

	EffectField* amount_val;
private:
	QOpenGLShader vert_shader;
	QOpenGLShader frag_shader;
};

#endif // INVERTEFFECT_H
