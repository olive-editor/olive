#include "nodegraph.h"

#include <QChildEvent>
#include <QDebug>

NodeGraph::NodeGraph()
{

}

void NodeGraph::childEvent(QChildEvent *event)
{
  if (event->type() == QEvent::ChildAdded || event->type() == QEvent::ChildRemoved) {
    emit NodeGraphChanged();
  }
}

void NodeGraph::SetOutputNode(Node *node)
{
  output_node_ = node;
}

Node *NodeGraph::OutputNode()
{
  return output_node_;
}
