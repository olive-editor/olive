#ifndef NODEIMAGEOUTPUT_H
#define NODEIMAGEOUTPUT_H

#include "nodes/node.h"

class NodeImageOutput : public Node
{
public:
  NodeImageOutput(Clip* c);

  virtual QString name() override;
  virtual QString id() override;
  virtual QString category() override;
  virtual QString description() override;
  virtual EffectType type() override;
  virtual olive::TrackType subtype() override;
  virtual NodePtr Create(Clip *c) override;
};

#endif // NODEIMAGEOUTPUT_H
