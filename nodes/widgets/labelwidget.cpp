#include "labelwidget.h"

LabelWidget::LabelWidget(Effect *parent, const QString &name, const QString &text) :
  EffectRow(parent, nullptr, name, false, false)
{
  AddField(new LabelField(this, text));
}
