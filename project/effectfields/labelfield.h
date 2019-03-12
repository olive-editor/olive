#ifndef LABELFIELD_H
#define LABELFIELD_H

#include "effectfield.h"

class LabelField : public EffectField
{
  Q_OBJECT
public:
  LabelField(EffectRow* parent, const QString& string);
};

#endif // LABELFIELD_H
