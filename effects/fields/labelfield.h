#ifndef LABELFIELD_H
#define LABELFIELD_H

#include "../effectfield.h"

class LabelField : public EffectField
{
  Q_OBJECT
public:
  LabelField(EffectRow* parent, const QString& string);

  virtual QWidget* CreateWidget(QWidget *existing = nullptr) override;
private:
  QString label_text_;
};

#endif // LABELFIELD_H
