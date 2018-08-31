#ifndef BOXBLUREFFECT_H
#define BOXBLUREFFECT_H

#include "../effect.h"

class BoxBlurEffect : public Effect
{
public:
    BoxBlurEffect(Clip* c);

    void process_gl(double timecode, GLTextureCoords &coords);
    void clean_gl();
private:
    QOpenGLShaderProgram program;
    QOpenGLShader vert;
    QOpenGLShader frag;

    EffectField* radius_val;
    EffectField* iteration_val;
    EffectField* horiz_val;
    EffectField* vert_val;
};

#endif // BOXBLUREFFECT_H
