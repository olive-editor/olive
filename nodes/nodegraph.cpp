#include "nodegraph.h"

#include <QDebug>

NodeGraph::NodeGraph() :
  output_node_(nullptr)
{

}

Node *NodeGraph::OutputNode()
{
  return output_node_.get();
}
