#ifndef FONTFIELD_H
#define FONTFIELD_H

#include "effectfield.h"

class FontField : public EffectField {
  Q_OBJECT
public:
  FontField(EffectRow* parent, const QString& id);

  QString GetFontAt(double timecode);
};

#endif // FONTFIELD_H
