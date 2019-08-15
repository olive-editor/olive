#ifndef ALPHAOVER_H
#define ALPHAOVER_H

#include "node/blend/blend.h"

class AlphaOverBlend : public BlendNode
{
public:
  AlphaOverBlend();

public slots:
  virtual void Process(const rational &time) override;
};

#endif // ALPHAOVER_H
