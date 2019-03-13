#ifndef COMBOFIELD_H
#define COMBOFIELD_H

#include "../effectfield.h"

struct ComboFieldItem {
  QString name;
  QVariant data;
};

class ComboField : public EffectField
{
  Q_OBJECT
public:
  ComboField(EffectRow* parent, const QString& id);

  void AddItem(const QString& text, const QVariant& data);

  virtual QWidget *CreateWidget() override;
  virtual void UpdateWidgetValue(QWidget* widget, double timecode) override;

signals:
  void IndexChanged(int i);

private:
  QVector<ComboFieldItem> items_;

private slots:
  void UpdateFromWidget(int index);
};

#endif // COMBOFIELD_H
