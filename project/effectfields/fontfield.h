#ifndef FONTFIELD_H
#define FONTFIELD_H

#include "effectfield.h"

class FontField : public EffectField {
  Q_OBJECT
public:
  FontField(EffectRow* parent, const QString& id);

  QString GetFontAt(double timecode);

  virtual QVariant ConvertStringToValue(const QString& s);
  virtual QString ConvertValueToString(const QVariant& v);
};

#endif // FONTFIELD_H
