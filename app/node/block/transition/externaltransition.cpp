#include "externaltransition.h"

ExternalTransition::ExternalTransition(const QString &xml_meta_filename) :
  meta_(xml_meta_filename)
{
  foreach (NodeInput* input, meta_.inputs()) {
    AddInput(input);
  }
}

Node *ExternalTransition::copy() const
{
  return new ExternalTransition(meta_.filename());
}

QString ExternalTransition::Name() const
{
  return meta_.Name();
}

QString ExternalTransition::id() const
{
  return meta_.id();
}

QString ExternalTransition::Category() const
{
  return meta_.Category();
}

QString ExternalTransition::Description() const
{
  return meta_.Description();
}

void ExternalTransition::Retranslate()
{
  meta_.Retranslate();
}

Node::Capabilities ExternalTransition::GetCapabilities(const NodeValueDatabase &) const
{
  return kShader;
}

QString ExternalTransition::ShaderVertexCode(const NodeValueDatabase &) const
{
  return meta_.vert_code();
}

QString ExternalTransition::ShaderFragmentCode(const NodeValueDatabase&) const
{
  return meta_.frag_code();
}

int ExternalTransition::ShaderIterations() const
{
  return meta_.iterations();
}

NodeInput *ExternalTransition::ShaderIterativeInput() const
{
  return meta_.iteration_input();
}
