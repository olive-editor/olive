#include "nodegraph.h"

#include <QDebug>

NodeGraph::NodeGraph() :
  output_node_(nullptr)
{

}

Effect *NodeGraph::OutputNode()
{
  return output_node_.get();
}
