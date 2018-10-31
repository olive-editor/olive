#include "transition.h"

#include <QOpenGLFunctions>

// TODO port to GLSL?

CrossDissolveTransition::CrossDissolveTransition() : Transition(VIDEO_DISSOLVE_TRANSITION) {}

void CrossDissolveTransition::process_transition(double progress) {
    float color[4];
    glGetFloatv(GL_CURRENT_COLOR, color);
	glColor4f(1.0, 1.0, 1.0, color[3]*progress);
}

Transition* CrossDissolveTransition::copy() {
	Transition* t = new CrossDissolveTransition();
	t->length = length;
	return t;
}
