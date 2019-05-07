#ifndef LABELWIDGET_H
#define LABELWIDGET_H

#include "nodes/nodeio.h"

class LabelWidget : public NodeIO
{
public:
  LabelWidget(OldEffectNode* parent, const QString& name, const QString& text);
};

#endif // LABELWIDGET_H
