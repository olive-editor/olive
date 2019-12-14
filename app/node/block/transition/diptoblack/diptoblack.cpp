#include "diptoblack.h"

Node *DipToBlackTransition::copy() const
{
  DipToBlackTransition* c = new DipToBlackTransition();

  CopyParameters(this, c);

  return c;
}

QString DipToBlackTransition::Name() const
{
  return tr("Dip to Black");
}

QString DipToBlackTransition::id() const
{
  return "org.olivevideoeditor.Olive.diptoblack";
}

QString DipToBlackTransition::Description() const
{
  return tr("A smooth dip to transparency and back into another clip.");
}

bool DipToBlackTransition::IsAccelerated() const
{
  return true;
}

QString DipToBlackTransition::AcceleratedCodeFragment() const
{
  return ReadFileAsString(":/shaders/diptoblack.frag");
}
