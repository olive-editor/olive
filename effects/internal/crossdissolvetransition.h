#ifndef CROSSDISSOLVETRANSITION_H
#define CROSSDISSOLVETRANSITION_H

#include "../effect.h"

class CrossDissolveTransition : public Effect {
public:
	CrossDissolveTransition(Clip* c, const EffectMeta* em);
	void process_coords(double timecode, GLTextureCoords &);
};

#endif // CROSSDISSOLVETRANSITION_H
