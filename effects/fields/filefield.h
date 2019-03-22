#ifndef FILEFIELD_H
#define FILEFIELD_H

#include "../effectfield.h"

class FileField : public EffectField
{
  Q_OBJECT
public:
  FileField(EffectRow* parent, const QString& id);

  QString GetFileAt(double timecode);

  virtual QWidget* CreateWidget(QWidget *existing = nullptr) override;
  virtual void UpdateWidgetValue(QWidget *widget, double timecode) override;
private slots:
  void UpdateFromWidget(const QString &s);
};

#endif // FILEFIELD_H
