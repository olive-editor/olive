#include "node.h"

#include "nodegraph.h"

Node::Node(NodeGraph *graph) :
  QObject(graph)
{
}

QString Node::name()
{
  return tr("Node");
}

QString Node::id()
{
  return "node";
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

NodeGraph *Node::ParentGraph()
{
  return static_cast<NodeGraph*>(parent());
}

double Node::Time()
{
  return ParentGraph()->Time();
}

const QPointF &Node::pos()
{
  return pos_;
}

void Node::SetPos(const QPointF &pos)
{
  pos_ = pos;
}
