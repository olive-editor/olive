#ifndef FONTFIELD_H
#define FONTFIELD_H

#include "combofield.h"

class FontField : public EffectField {
  Q_OBJECT
public:
  FontField(EffectRow* parent, const QString& id);

  QString GetFontAt(double timecode);

  virtual QWidget *CreateWidget(QWidget *existing = nullptr) override;
  virtual void UpdateWidgetValue(QWidget* widget, double timecode) override;

private:
  QStringList font_list;
private slots:
  void UpdateFromWidget(const QString& index);
};

#endif // FONTFIELD_H
