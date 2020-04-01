#include "colorwheelwidget.h"

#include <QPainter>
#include <QtMath>

#define RADIAN_TO_0_1_RANGE 0.15915494309188486110703542313983

ColorWheelWidget::ColorWheelWidget(QWidget *parent) :
  QWidget(parent)
{

}

void ColorWheelWidget::paintEvent(QPaintEvent *event)
{
  QWidget::paintEvent(event);

  QPainter p(this);

  qreal radius = qMin(width(), height()) / 2;
  QPoint center = rect().center();

  p.setPen(Qt::red);

  int x_start = center.x() - radius;
  int x_end= center.x() + radius;
  int y_start = center.y() - radius;
  int y_end= center.y() + radius;

  for (int i=x_start;i<x_end;i++) {
    for (int j=y_start;j<y_end;j++) {
      qreal opposite = j - center.y();
      qreal adjacent = i - center.x();
      qreal pix_len = qSqrt(qPow(adjacent, 2) + qPow(opposite, 2));

      if (pix_len <= radius) {
        qreal hue = qAtan2(opposite, adjacent) * RADIAN_TO_0_1_RANGE + 0.5;
        qreal sat = (pix_len / radius);
        qreal value = 1.0;

        QColor c;
        c.setHsvF(hue, sat, value);
        p.setPen(c);

        p.drawPoint(i, j);
      }
    }
  }
}
