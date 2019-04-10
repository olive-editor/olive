#ifndef LABELWIDGET_H
#define LABELWIDGET_H

#include "effects/effectrow.h"

class LabelWidget : public EffectRow
{
public:
  LabelWidget(Effect* parent, const QString& name, const QString& text);
};

#endif // LABELWIDGET_H
