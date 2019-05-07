#include "labelwidget.h"

LabelWidget::LabelWidget(OldEffectNode *parent, const QString &name, const QString &text) :
  NodeIO(parent, nullptr, name, false, false)
{
  AddField(new LabelField(this, text));
}
