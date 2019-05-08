#include "nodegraph.h"

#include <QDebug>

NodeGraph::NodeGraph() :
  output_node_(nullptr)
{

}

void NodeGraph::AddNode(NodePtr node)
{
  nodes_.append(node);
  emit NodeGraphChanged();
}

Node *NodeGraph::OutputNode()
{
  return output_node_.get();
}
