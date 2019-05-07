#include "nodeedge.h"

#include <QDebug>

#include "nodes/nodeio.h"

NodeEdge::NodeEdge(NodeIO *output, NodeIO *input) :
  output_(output),
  input_(input)
{
}

NodeIO *NodeEdge::output()
{
  return output_;
}

NodeIO *NodeEdge::input()
{
  return input_;
}
