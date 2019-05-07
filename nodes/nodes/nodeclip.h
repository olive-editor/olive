#ifndef NODECLIP_H
#define NODECLIP_H

#include "nodes/node.h"

class NodeClip : public Node
{
public:
  NodeClip(NodeGraph* graph);

  long in();
  long out();
  long clip_in();

  void set_in(long in);
  void set_out(long out);
  void set_clip_in(long clip_in);

private:
  long in_;
  long out_;
  long clip_in_;
};

#endif // NODECLIP_H
