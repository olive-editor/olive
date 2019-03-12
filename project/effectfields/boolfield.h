#ifndef BOOLFIELD_H
#define BOOLFIELD_H

#include "effectfield.h"

class BoolField : public EffectField
{
  Q_OBJECT
public:
  BoolField(EffectRow* parent, const QString& id);

  bool GetBoolAt(double timecode);

  virtual QWidget* CreateWidget() override;

  virtual QVariant ConvertStringToValue(const QString& s) override;
  virtual QString ConvertValueToString(const QVariant& v) override;
signals:
  void Toggled(bool);
private slots:
  void UpdateFromWidget(bool b);
};

#endif // BOOLFIELD_H
