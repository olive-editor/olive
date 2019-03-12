#ifndef FONTFIELD_H
#define FONTFIELD_H

#include "effectfield.h"

class FontField : public EffectField {
  Q_OBJECT
public:
  FontField(EffectRow* parent, const QString& id);

  QString GetFontAt(double timecode);

  virtual QWidget *CreateWidget() override;
private:
  QStringList font_list;
private slots:
  void UpdateFromWidget(const QString& index);
};

#endif // FONTFIELD_H
