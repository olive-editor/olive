#include "crossdissolve.h"

CrossDissolveTransition::CrossDissolveTransition()
{

}

Node *CrossDissolveTransition::copy() const
{
  return new CrossDissolveTransition();
}

QString CrossDissolveTransition::Name() const
{
  return tr("Cross Dissolve");
}

QString CrossDissolveTransition::id() const
{
  return "org.olivevideoeditor.Olive.crossdissolve";
}

QString CrossDissolveTransition::Description() const
{
  return tr("A smooth fade transition from one video clip to another.");
}

bool CrossDissolveTransition::IsAccelerated() const
{
  return true;
}

QString CrossDissolveTransition::CodeFragment() const
{
  return ReadFileAsString(":/shaders/crossdissolve.frag");
}
