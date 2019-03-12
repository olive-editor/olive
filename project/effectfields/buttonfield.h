#ifndef BUTTONFIELD_H
#define BUTTONFIELD_H

#include "effectfield.h"

class ButtonField : public EffectField
{
  Q_OBJECT
public:
  ButtonField(EffectRow* parent, const QString& string);

  void SetCheckable(bool c);
  void SetChecked(bool c);
signals:
  void CheckedChanged(bool);
private:
  bool checkable_;
  bool checked_;
};

#endif // BUTTONFIELD_H
