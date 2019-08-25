#ifndef BLEND_H
#define BLEND_H

#include "node/node.h"

class BlendNode : public Node
{
public:
  BlendNode();

  virtual QString Category() override;

  NodeInput* base_input();

  NodeInput* blend_input();

  NodeOutput* texture_output();

private:
  NodeInput* base_input_;

  NodeInput* blend_input_;

  NodeOutput* texture_output_;
};

#endif // BLEND_H
