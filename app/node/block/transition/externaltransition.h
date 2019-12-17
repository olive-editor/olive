#ifndef EXTERNALTRANSITION_H
#define EXTERNALTRANSITION_H

#include "transition.h"

#include "node/metareader.h"

class ExternalTransition : public TransitionBlock
{
public:
  ExternalTransition(const QString& xml_meta_filename);

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual bool IsAccelerated() const override;
  virtual QString AcceleratedCodeVertex() const override;
  virtual QString AcceleratedCodeFragment() const override;
  virtual int AcceleratedCodeIterations() const override;
  virtual NodeInput* AcceleratedCodeIterativeInput() const override;

private:
  NodeMetaReader meta_;
};

#endif // EXTERNALTRANSITION_H
