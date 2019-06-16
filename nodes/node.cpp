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

const rational& Node::Time()
{
  return 0;
}

void Node::Process(const rational &time)
{
  Q_UNUSED(time)
}

NodeGraph *Node::ParentGraph()
{
  return static_cast<NodeGraph*>(parent());
}

const QPointF &Node::pos()
{
  return pos_;
}

void Node::SetPos(const QPointF &pos)
{
  pos_ = pos;
}
