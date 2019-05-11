#include "node.h"

#include "nodegraph.h"

Node::Node(NodeGraph *graph) :
  QObject(graph)
{
}

void Node::AddParameter(NodeIO *param)
{
  param->setParent(this);
  parameters_.append(param);
}

int Node::IndexOfParameter(NodeIO *param)
{
  return parameters_.indexOf(param);
}

NodeIO *Node::Parameter(int i)
{
  return parameters_.at(i);
}

int Node::ParameterCount()
{
  return parameters_.size();
}
