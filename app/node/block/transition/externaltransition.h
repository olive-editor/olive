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

  virtual Capabilities GetCapabilities(const NodeValueDatabase&) const override;
  virtual QString ShaderVertexCode(const NodeValueDatabase&) const override;
  virtual QString ShaderFragmentCode(const NodeValueDatabase&) const override;
  virtual int ShaderIterations() const override;
  virtual NodeInput* ShaderIterativeInput() const override;

private:
  NodeMetaReader meta_;
};

#endif // EXTERNALTRANSITION_H
