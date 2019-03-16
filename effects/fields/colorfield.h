#ifndef COLORFIELD_H
#define COLORFIELD_H

#include "../effectfield.h"

class ColorField : public EffectField
{
  Q_OBJECT
public:
  ColorField(EffectRow* parent, const QString& id);

  QColor GetColorAt(double timecode);

  virtual QWidget* CreateWidget(QWidget *existing = nullptr) override;
  virtual void UpdateWidgetValue(QWidget* widget, double timecode) override;

  virtual QVariant ConvertStringToValue(const QString& s) override;
  virtual QString ConvertValueToString(const QVariant& v) override;
private slots:
  void UpdateFromWidget(const QColor &c);
};

#endif // COLORFIELD_H
