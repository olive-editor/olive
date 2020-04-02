#ifndef COLORPREVIEWBOX_H
#define COLORPREVIEWBOX_H

#include <QWidget>

#include "render/color.h"

class ColorPreviewBox : public QWidget
{
  Q_OBJECT
public:
  ColorPreviewBox(QWidget* parent = nullptr);

public slots:
  void SetColor(const Color& c);

protected:
  virtual void paintEvent(QPaintEvent* e) override;

private:
  Color color_;

};

#endif // COLORPREVIEWBOX_H
