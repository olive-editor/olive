#ifndef NODESHADER_H
#define NODESHADER_H

#include "effects/effect.h"

class NodeShader : public Effect {
  Q_OBJECT
public:
  NodeShader(Clip *c, const EffectMeta *em);
};

#endif // NODESHADER_H
