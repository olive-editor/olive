#include "transition.h"

#include <QOpenGLFunctions>

CrossDissolveTransition::CrossDissolveTransition() : Transition(VIDEO_DISSOLVE_TRANSITION, "Cross Dissolve") {}

void CrossDissolveTransition::process_transition(float progress) {
    float color[4];
    glGetFloatv(GL_CURRENT_COLOR, color);
    glColor4f(1.0, 1.0, 1.0, color[3]*progress);
}

Transition* CrossDissolveTransition::copy() {
	Transition* t = new CrossDissolveTransition();
	t->length = length;
	return t;
}
