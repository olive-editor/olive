#ifndef CROSSDISSOLVETRANSITION_H
#define CROSSDISSOLVETRANSITION_H

#include "project/transition.h"

class CrossDissolveTransition : public Transition {
public:
	CrossDissolveTransition(Clip* c, const EffectMeta* em);
	void process_coords(double timecode, GLTextureCoords &);
};

#endif // CROSSDISSOLVETRANSITION_H
