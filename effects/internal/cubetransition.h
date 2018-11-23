#ifndef CUBETRANSITION_H
#define CUBETRANSITION_H

#include "project/transition.h"

class CubeTransition : public Transition {
public:
    CubeTransition(Clip* c, Clip* s, const EffectMeta* em);
    void process_coords(double timecode, GLTextureCoords &, int data);
};

#endif // CUBETRANSITION_H
