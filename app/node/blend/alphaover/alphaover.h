#ifndef ALPHAOVER_H
#define ALPHAOVER_H

#include "node/blend/blend.h"

class AlphaOverBlend : public BlendNode
{
public:
  AlphaOverBlend();

  virtual QString Name() override;
  virtual QString id() override;
  virtual QString Description() override;

protected:
  virtual void Process(const rational &time) override;
};

#endif // ALPHAOVER_H
