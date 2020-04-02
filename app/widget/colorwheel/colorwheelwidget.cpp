#include "colorwheelwidget.h"

#include <QPainter>
#include <QtMath>

#include "node/node.h"

#define M_180_OVER_PI 57.295791433133264917914229473464
#define M_RADIAN_TO_0_1 0.15915497620314795810531730409296

ColorWheelWidget::ColorWheelWidget(QWidget *parent) :
  ColorSwatchWidget(parent),
  val_(1.0f),
  force_redraw_(false)
{
}

Color ColorWheelWidget::GetColorFromScreenPos(const QPoint &p) const
{
  return GetColorFromTriangle(GetTriangleFromCoords(rect().center(), p));
}

void ColorWheelWidget::resizeEvent(QResizeEvent *e)
{
  ColorSwatchWidget::resizeEvent(e);

  emit DiameterChanged(GetDiameter());
}

void ColorWheelWidget::paintEvent(QPaintEvent *e)
{
  ColorSwatchWidget::paintEvent(e);

  int diameter = GetDiameter();

  // Half diameter (add one to ensure the division rounds up)
  int radius = (diameter + 1) / 2;

  if (cached_wheel_.width() != diameter || force_redraw_) {
    cached_wheel_ = QPixmap(QSize(diameter, diameter));
    cached_wheel_.fill(Qt::transparent);
    force_redraw_ = false;

    QPainter p(&cached_wheel_);
    QPoint center(radius, radius);

    for (int i=0;i<diameter;i++) {
      for (int j=0;j<diameter;j++) {
        Triangle tri = GetTriangleFromCoords(center, j, i);

        if (tri.hypotenuse <= radius) {
          QColor c = GetColorFromTriangle(tri).toQColor();

          // Very basic antialiasing around the edges of the wheel
          qreal alpha = qMin(1.0, radius - tri.hypotenuse);
          c.setAlphaF(alpha);

          p.setPen(c);

          p.drawPoint(i, j);
        }
      }
    }
  }

  QPainter p(this);

  // Draw wheel pixmap
  int x, y;

  if (width() == height()) {
    x = 0;
    y = 0;
  } else if (width() > height()) {
    x = (width() - height()) / 2;
    y = 0;
  } else {
    x = 0;
    y = (height() - width()) / 2;
  }

  p.drawPixmap(x, y, cached_wheel_);


  // Draw selection
  // Really rough algorithm for determining whether the selector UI should be white or black


  int selector_radius = qMax(1, radius / 32);
  p.setPen(QPen(GetUISelectorColor(), qMax(1, selector_radius / 4)));
  p.setBrush(Qt::NoBrush);

  p.drawEllipse(GetCoordsFromColor(GetSelectedColor()), selector_radius, selector_radius);
}

void ColorWheelWidget::SelectedColorChangedEvent(const Color &c)
{
  force_redraw_ = true;
  val_ = c.value();
}

int ColorWheelWidget::GetDiameter() const
{
  return qMin(width(), height());
}

qreal ColorWheelWidget::GetRadius() const
{
  return GetDiameter() * 0.5;
}

ColorWheelWidget::Triangle ColorWheelWidget::GetTriangleFromCoords(const QPoint &center, const QPoint &p) const
{
  return GetTriangleFromCoords(center, p.y(), p.x());
}

ColorWheelWidget::Triangle ColorWheelWidget::GetTriangleFromCoords(const QPoint &center, qreal y, qreal x) const
{
  qreal opposite = y - center.y();
  qreal adjacent = x - center.x();
  qreal hypotenuse = qSqrt(qPow(adjacent, 2) + qPow(opposite, 2));

  return {opposite, adjacent, hypotenuse};
}

Color ColorWheelWidget::GetColorFromTriangle(const ColorWheelWidget::Triangle &tri) const
{
  qreal hue = qAtan2(tri.opposite, tri.adjacent) * M_180_OVER_PI + 180.0;
  qreal sat = qMin(1.0, (tri.hypotenuse / GetRadius()));

  return Color::fromHsv(hue, sat, val_);
}

QPoint ColorWheelWidget::GetCoordsFromColor(const Color &c) const
{
  float hue, sat, val;
  c.toHsv(&hue, &sat, &val);

  qreal hypotenuse = sat * GetRadius();

  qreal radian_angle = (hue - 180.0) / M_180_OVER_PI;

  qreal opposite = qSin(radian_angle) * hypotenuse;

  qreal adjacent = qCos(radian_angle) * hypotenuse;

  QPoint pos(qRound(adjacent), qRound(opposite));

  pos += rect().center();

  return pos;
}
