#ifndef NODEVIDEOCLIP_H
#define NODEVIDEOCLIP_H

#include "nodeblock.h"

class NodeVideoClip : public NodeBlock
{
public:
  NodeVideoClip(NodeGraph* parent);

  NodeIO* texture_input();
  NodeIO* texture_output();

private:
  NodeIO* texture_input_;
  NodeIO* texture_output_;
};

#endif // NODEVIDEOCLIP_H
