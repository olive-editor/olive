#include "cubetransition.h"

#include "debug.h"

CubeTransition::CubeTransition(Clip* c, Clip* s, const EffectMeta* em) : Transition(c, s, em) {
    enable_coords = true;
}

void CubeTransition::process_coords(double progress, GLTextureCoords& coords, int data) {

    coords.vertexTopLeftZ = 1;
    coords.vertexBottomLeftZ = 1;
}
