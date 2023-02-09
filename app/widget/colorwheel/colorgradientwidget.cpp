/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "colorgradientwidget.h"

#include <QPainter>

#include "common/lerp.h"
#include "node/node.h"

namespace olive {

ColorGradientWidget::ColorGradientWidget(Qt::Orientation orientation, QWidget *parent) :
  ColorSwatchWidget(parent),
  orientation_(orientation),
  val_(1.0)
{
}

Color ColorGradientWidget::GetColorFromScreenPos(const QPoint &p) const
{
  if (orientation_ == Qt::Horizontal) {
    return LerpColor(start_, end_, p.x(), width());
  } else {
    return LerpColor(start_, end_, p.y(), height());
  }
}

void ColorGradientWidget::paintEvent(QPaintEvent *e)
{
  QWidget::paintEvent(e);

  QPainter p(this);

  int min;
  int max;

  if (orientation_ == Qt::Horizontal) {
    min = height();
    max = width();
  } else {
    min = width();
    max = height();
  }

  for (int i=0;i<max;i++) {
    p.setPen(QtUtils::toQColor(GetManagedColor(LerpColor(start_, end_, i, max))));

    if (orientation_ == Qt::Horizontal) {
      p.drawLine(i, 0, i, height());
    } else {
      p.drawLine(0, i, width(), i);
    }
  }

  // Draw selector
  int selector_radius = qMax(2, min / 8);
  p.setPen(QPen(GetUISelectorColor(), qMax(1, selector_radius / 2)));
  p.setBrush(Qt::NoBrush);

  float clamped_val = std::clamp(val_, 0.0f, 1.0f);

  if (orientation_ == Qt::Horizontal) {
    p.drawRect(qRound(width() * (1.0 - clamped_val)) - selector_radius, 0, selector_radius * 2, height() - 1);
  } else {
    p.drawRect(0, qRound(height() * (1.0 - clamped_val)) - selector_radius, width() - 1, selector_radius * 2);
  }
}

void ColorGradientWidget::SelectedColorChangedEvent(const Color &c, bool external)
{
  float hue, sat;

  c.toHsv(&hue, &sat, &val_);

  if (external) {
    start_ = Color::fromHsv(hue, sat, 1.0);
    end_ = Color::fromHsv(hue, sat, 0.0);
  }
}

Color ColorGradientWidget::LerpColor(const Color &a, const Color &b, int i, int max)
{
  float t = std::clamp(static_cast<float>(i) / static_cast<float>(max), 0.0f, 1.0f);

  return Color(lerp(a.red(), b.red(), t),
               lerp(a.green(), b.green(), t),
               lerp(a.blue(), b.blue(), t));
}

}
