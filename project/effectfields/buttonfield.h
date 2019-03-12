#ifndef BUTTONFIELD_H
#define BUTTONFIELD_H

#include "effectfield.h"

class ButtonField : public EffectField
{
  Q_OBJECT
public:
  ButtonField(EffectRow* parent, const QString& string);

  void SetCheckable(bool c);
  virtual QWidget* CreateWidget() override;

public slots:
  void SetChecked(bool c);

signals:
  void CheckedChanged(bool);
  void Toggled(bool);

private:
  bool checkable_;
  bool checked_;

  QString button_text_;
};

#endif // BUTTONFIELD_H
