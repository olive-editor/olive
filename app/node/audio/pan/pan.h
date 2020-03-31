#ifndef PANNODE_H
#define PANNODE_H

#include "node/node.h"

class PanNode : public Node
{
public:
  PanNode();

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Category() const override;
  virtual QString Description() const override;

  virtual Capabilities GetCapabilities(const NodeValueDatabase&) const override;
  virtual NodeInput* ProcessesSamplesFrom(const NodeValueDatabase &value) const override;
  virtual void ProcessSamples(const NodeValueDatabase& values, const AudioRenderingParams& params, const SampleBufferPtr input, SampleBufferPtr output, int index) const override;

  virtual void Retranslate() override;

private:
  NodeInput* samples_input_;
  NodeInput* panning_input_;

};

#endif // PANNODE_H
