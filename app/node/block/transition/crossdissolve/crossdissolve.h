#ifndef CROSSDISSOLVETRANSITION_H
#define CROSSDISSOLVETRANSITION_H

#include "../transition.h"

class CrossDissolveTransition : public TransitionBlock
{
public:
  CrossDissolveTransition();

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Description() const override;

  virtual bool IsAccelerated() const override;
  virtual QString CodeFragment() const override;
};

#endif // CROSSDISSOLVETRANSITION_H
