#ifndef SOLIDEFFECT_H
#define SOLIDEFFECT_H

#include "../effect.h"

class SolidEffect : public Effect {
	Q_OBJECT
public:
	SolidEffect(Clip* c);
	EffectField* solid_type;
	EffectField* solid_color_field;
	EffectField* opacity_field;
	void process_gl(double timecode, GLTextureCoords& coords);
    void clean_gl();
    void process_image(long p, uint8_t* data, int width, int height);
private:
    QOpenGLShaderProgram program;
    QOpenGLShader vert;
    QOpenGLShader frag;
};

#endif // SOLIDEFFECT_H
