#ifndef MEDIANODE_H
#define MEDIANODE_H

#include "nodes/node.h"

class NodeMedia : public Node
{
public:
  NodeMedia(Clip *c);

  virtual QString name() override;
  virtual QString id() override;
  virtual QString category() override;
  virtual QString description() override;
  virtual EffectType type() override;
  virtual olive::TrackType subtype() override;
  virtual NodePtr Create(Clip *c) override;
};

#endif // MEDIANODE_H
