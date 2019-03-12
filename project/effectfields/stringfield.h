#ifndef STRINGFIELD_H
#define STRINGFIELD_H

#include "effectfield.h"

class StringField : public EffectField
{
  Q_OBJECT
public:
  StringField(EffectRow* parent, const QString& id);

  QString GetStringAt(double timecode);
};

#endif // STRINGFIELD_H
