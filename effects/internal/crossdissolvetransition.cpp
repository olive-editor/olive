#include "crossdissolvetransition.h"

#include <QOpenGLFunctions>

CrossDissolveTransition::CrossDissolveTransition(Clip* c, Clip* s, const EffectMeta* em) : Transition(c, s, em) {
	enable_coords = true;
}

void CrossDissolveTransition::process_coords(double progress, GLTextureCoords&, int data) {
    if (!(data == TA_CLOSING_TRANSITION && secondary_clip != NULL)) {
        float color[4];
        glGetFloatv(GL_CURRENT_COLOR, color);
        if (data == TA_CLOSING_TRANSITION) progress = 1.0 - progress;
        glColor4f(1.0, 1.0, 1.0, color[3]*progress);
    }
}
