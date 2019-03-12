#ifndef FILEFIELD_H
#define FILEFIELD_H

#include "effectfield.h"

class FileField : public EffectField
{
  Q_OBJECT
public:
  FileField(EffectRow* parent, const QString& id);

  QString GetFileAt(double timecode);
};

#endif // FILEFIELD_H
