#ifndef BOOLFIELD_H
#define BOOLFIELD_H

#include "effectfield.h"

class BoolField : public EffectField
{
  Q_OBJECT
public:
  BoolField(EffectRow* parent, const QString& id);

  bool GetBoolAt(double timecode);

  virtual QVariant ConvertStringToValue(const QString& s);
  virtual QString ConvertValueToString(const QVariant& v);
};

#endif // BOOLFIELD_H
