#include "external.h"

#include <QFile>

ExternalNode::ExternalNode(const QString &xml_meta_filename) :
  meta_(xml_meta_filename)
{
  foreach (NodeInput* input, meta_.inputs()) {
    AddInput(input);
  }
}

Node *ExternalNode::copy() const
{
  return new ExternalNode(meta_.filename());
}

QString ExternalNode::Name() const
{
  return meta_.Name();
}

QString ExternalNode::id() const
{
  return meta_.id();
}

QString ExternalNode::Category() const
{
  return meta_.Category();
}

QString ExternalNode::Description() const
{
  return meta_.Description();
}

void ExternalNode::Retranslate()
{
  meta_.Retranslate();
}

bool ExternalNode::IsAccelerated() const
{
  return true;
}

QString ExternalNode::AcceleratedCodeVertex() const
{
  return meta_.vert_code();
}

QString ExternalNode::AcceleratedCodeFragment() const
{
  return meta_.frag_code();
}

int ExternalNode::AcceleratedCodeIterations() const
{
  return meta_.iterations();
}

NodeInput *ExternalNode::AcceleratedCodeIterativeInput() const
{
  return meta_.iteration_input();
}
