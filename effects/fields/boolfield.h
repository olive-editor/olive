#ifndef BOOLFIELD_H
#define BOOLFIELD_H

#include "../effectfield.h"

/**
 * @brief The BoolField class
 *
 * An EffectField derivative the uses boolean values (true or false) and uses a checkbox as its visual representation.
 */
class BoolField : public EffectField
{
  Q_OBJECT
public:
  BoolField(EffectRow* parent, const QString& id);

  bool GetBoolAt(double timecode);

  virtual QWidget* CreateWidget(QWidget *existing = nullptr) override;
  virtual void UpdateWidgetValue(QWidget* widget, double timecode) override;

  virtual QVariant ConvertStringToValue(const QString& s) override;
  virtual QString ConvertValueToString(const QVariant& v) override;
signals:
  void Toggled(bool);
private slots:
  void UpdateFromWidget(bool b);
};

#endif // BOOLFIELD_H
