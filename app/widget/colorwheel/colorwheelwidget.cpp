/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "colorwheelwidget.h"

#include <QPainter>
#include <QtMath>

#include "common/clamp.h"
#include "node/node.h"

OLIVE_NAMESPACE_ENTER

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

  // Half diameter
  int radius = diameter / 2;

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
          Color managed = GetManagedColor(GetColorFromTriangle(tri));
          QColor c = managed.toQColor();

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

void ColorWheelWidget::SelectedColorChangedEvent(const Color &c, bool external)
{
  if (external) {
    force_redraw_ = true;
    val_ = clamp(c.value(), 0.0, 1.0);
  }
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
  double hue, sat, val;
  c.toHsv(&hue, &sat, &val);

  qreal hypotenuse = sat * GetRadius();

  qreal radian_angle = (hue - 180.0) / M_180_OVER_PI;

  qreal opposite = qSin(radian_angle) * hypotenuse;

  qreal adjacent = qCos(radian_angle) * hypotenuse;

  QPoint pos(qRound(adjacent), qRound(opposite));

  pos += rect().center();

  return pos;
}

OLIVE_NAMESPACE_EXIT
