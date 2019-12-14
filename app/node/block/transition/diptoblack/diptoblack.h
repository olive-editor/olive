#ifndef DIPTOBLACKTRANSITION_H
#define DIPTOBLACKTRANSITION_H

#include "../transition.h"

class DipToBlackTransition : public TransitionBlock
{
public:
  DipToBlackTransition() = default;

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Description() const override;

  virtual bool IsAccelerated() const override;
  virtual QString AcceleratedCodeFragment() const override;
};

#endif // DIPTOBLACKTRANSITION_H
