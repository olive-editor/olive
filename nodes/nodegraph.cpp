#include "nodegraph.h"

#include <QChildEvent>
#include <QDebug>

NodeGraph::NodeGraph() :
  output_node_(nullptr),
  ctx_(nullptr)
{

}

void NodeGraph::Process()
{
  if (output_node_ != nullptr) {
    output_node_->Process(Time());
  }
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

const rational& NodeGraph::Time()
{
  return time_;
}

void NodeGraph::SetTime(const rational &d)
{
  time_ = d;
  emit TimeChanged();
}

MemoryCache *NodeGraph::memory_cache()
{
  return &memory_cache_;
}

const int &NodeGraph::width()
{
  return width_;
}

const int &NodeGraph::height()
{
  return height_;
}

void NodeGraph::set_width(const int& w)
{
  width_ = w;
  emit ParametersChanged();
}

void NodeGraph::set_height(const int& h)
{
  height_ = h;
  emit ParametersChanged();
}

void NodeGraph::SetGLContext(QOpenGLContext *ctx)
{
  memory_cache_.SetParameters(ctx, width_, height_);
}
