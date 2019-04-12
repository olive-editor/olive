#include "nodeedge.h"

#include <QDebug>

#include "effects/effectrow.h"

NodeEdge::NodeEdge(EffectRow *output, EffectRow *input) :
  output_(output),
  input_(input)
{
}

EffectRow *NodeEdge::output()
{
  return output_;
}

EffectRow *NodeEdge::input()
{
  return input_;
}
