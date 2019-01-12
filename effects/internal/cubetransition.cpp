#include "cubetransition.h"

#include "debug.h"

CubeTransition::CubeTransition(Clip* c, Clip* s, const EffectMeta* em) : Transition(c, s, em) {
	enable_coords = true;
}

void CubeTransition::process_coords(double, GLTextureCoords& coords, int) {

	coords.vertexTopLeftZ = 1;
	coords.vertexBottomLeftZ = 1;
}
