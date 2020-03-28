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

Node::Capabilities ExternalNode::GetCapabilities(const NodeValueDatabase &) const
{
  return kShader;
}

QString ExternalNode::ShaderVertexCode(const NodeValueDatabase&) const
{
  return meta_.vert_code();
}

QString ExternalNode::ShaderFragmentCode(const NodeValueDatabase&) const
{
  return meta_.frag_code();
}

int ExternalNode::ShaderIterations() const
{
  return meta_.iterations();
}

NodeInput *ExternalNode::ShaderIterativeInput() const
{
  return meta_.iteration_input();
}
