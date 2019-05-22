#ifndef MEDIANODE_H
#define MEDIANODE_H

#include "nodes/node.h"
#include "rendering/memorycache.h"

class NodeMedia : public Node
{
  Q_OBJECT
public:
  NodeMedia(NodeGraph *c);

  virtual QString name() override;
  virtual QString id() override;

  virtual void Process(const rational& time) override;

  NodeIO* matrix_input();
  NodeIO* texture_output();

private:
  NodeIO* matrix_input_;
  NodeIO* texture_output_;

  MemoryCache::Reference buffer_;
};

#endif // MEDIANODE_H
