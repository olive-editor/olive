#ifndef STRINGFIELD_H
#define STRINGFIELD_H

#include "../effectfield.h"

class StringField : public EffectField
{
  Q_OBJECT
public:
  StringField(EffectRow* parent, const QString& id);

  QString GetStringAt(double timecode);

  virtual QWidget *CreateWidget() override;
private slots:
  void UpdateFromWidget(const QString& b);
};

#endif // STRINGFIELD_H
