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

double NodeGraph::Time()
{
  return time_;
}

void NodeGraph::SetTime(double d)
{
  time_ = d;
  emit TimeChanged();
}
