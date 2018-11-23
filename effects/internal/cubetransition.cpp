#include "cubetransition.h"

#include "debug.h"

CubeTransition::CubeTransition(Clip* c, Clip* s, const EffectMeta* em) : Transition(c, s, em) {
    enable_coords = true;
}

void CubeTransition::process_coords(double progress, GLTextureCoords&, int data) {
    glTranslatef(-progress, 0, -progress);
}
