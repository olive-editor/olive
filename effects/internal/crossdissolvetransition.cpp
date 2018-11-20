#include "crossdissolvetransition.h"

#include <QOpenGLFunctions>

CrossDissolveTransition::CrossDissolveTransition(Clip* c, const EffectMeta* em) : Transition(c, em) {
	enable_coords = true;
}

void CrossDissolveTransition::process_coords(double progress, GLTextureCoords&) {
    float color[4];
    glGetFloatv(GL_CURRENT_COLOR, color);
	glColor4f(1.0, 1.0, 1.0, color[3]*progress);
}
