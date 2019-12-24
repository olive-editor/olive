#ifndef VOLUMENODE_H
#define VOLUMENODE_H

#include "node/node.h"

class VolumeNode : public Node
{
public:
  VolumeNode();

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Category() const override;
  virtual QString Description() const override;

  virtual bool ProcessesSamples() const override;
  virtual void ProcessSamples(const NodeValueDatabase& values, const AudioRenderingParams& params, const float* input, float* output, int index) const override;

private:
  NodeInput* samples_input_;
  NodeInput* volume_input_;

};

#endif // VOLUMENODE_H
