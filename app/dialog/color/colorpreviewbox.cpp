#include "colorpreviewbox.h"

#include <QPainter>

ColorPreviewBox::ColorPreviewBox(QWidget *parent) :
  QWidget(parent)
{
}

void ColorPreviewBox::SetColor(const Color &c)
{
  color_ = c;
  update();
}

void ColorPreviewBox::paintEvent(QPaintEvent *e)
{
  QWidget::paintEvent(e);

  QPainter p(this);

  p.setPen(Qt::black);

  QColor c;
  c.setRedF(color_.red());
  c.setGreenF(color_.green());
  c.setBlueF(color_.blue());
  p.setBrush(c);

  p.drawRect(rect());
}
