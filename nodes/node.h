#ifndef NODE_H
#define NODE_H

#include <memory>

class NodeGraph;

class Node;
using NodePtr = std::shared_ptr<Node>;

class Node
{
public:
  Node(NodeGraph* parent);

private:
  NodeGraph* parent_;
};

#endif // NODE_H
