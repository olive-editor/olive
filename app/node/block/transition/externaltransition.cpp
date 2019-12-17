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
  ExternalTransition* t = new ExternalTransition(meta_.filename());

  CopyParameters(this, t);

  return t;
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

bool ExternalTransition::IsAccelerated() const
{
  return true;
}

QString ExternalTransition::AcceleratedCodeVertex() const
{
  return meta_.vert_code();
}

QString ExternalTransition::AcceleratedCodeFragment() const
{
  return meta_.frag_code();
}

int ExternalTransition::AcceleratedCodeIterations() const
{
  return meta_.iterations();
}

NodeInput *ExternalTransition::AcceleratedCodeIterativeInput() const
{
  return meta_.iteration_input();
}
