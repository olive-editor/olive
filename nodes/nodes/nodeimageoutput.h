#ifndef NODEIMAGEOUTPUT_H
#define NODEIMAGEOUTPUT_H

#include "nodes/oldeffectnode.h"

class NodeImageOutput : public OldEffectNode
{
public:
  NodeImageOutput(Clip* c);

  virtual QString name() override;
  virtual QString id() override;
  virtual QString category() override;
  virtual QString description() override;
  virtual EffectType type() override;
  virtual olive::TrackType subtype() override;
  virtual OldEffectNodePtr Create(Clip *c) override;
};

#endif // NODEIMAGEOUTPUT_H
