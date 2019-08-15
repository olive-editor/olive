#ifndef BLEND_H
#define BLEND_H

#include "node/node.h"

class BlendNode : public Node
{
public:
  BlendNode();

  NodeInput* base_input();

  NodeInput* blend_input();

private:
  NodeInput* base_input_;

  NodeInput* blend_input_;
};

#endif // BLEND_H
