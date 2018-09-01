#ifndef INVERTEFFECT_H
#define INVERTEFFECT_H

#include "../effect.h"

class InvertEffect : public Effect {
	Q_OBJECT
public:
	InvertEffect(Clip* c);
	void process_gl(double timecode, GLTextureCoords& coords);
private:
	EffectField* amount_val;
};

#endif // INVERTEFFECT_H
