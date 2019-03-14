#ifndef STRINGFIELD_H
#define STRINGFIELD_H

#include "../effectfield.h"

class StringField : public EffectField
{
  Q_OBJECT
public:
  StringField(EffectRow* parent, const QString& id, bool rich_text = true);

  QString GetStringAt(double timecode);

  virtual QWidget *CreateWidget() override;
  virtual void UpdateWidgetValue(QWidget* widget, double timecode) override;
private slots:
  void UpdateFromWidget(const QString& b);
private:
  bool rich_text_;
};

#endif // STRINGFIELD_H
