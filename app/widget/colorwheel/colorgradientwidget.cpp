#include "colorgradientwidget.h"

#include <QPainter>

#include "common/clamp.h"
#include "common/lerp.h"
#include "node/node.h"

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
    p.setPen(GetManagedColor(LerpColor(start_, end_, i, max)).toQColor());

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

  if (orientation_ == Qt::Horizontal) {
    p.drawRect(qRound(width() * (1.0 - val_)) - selector_radius, 0, selector_radius * 2, height() - 1);
  } else {
    p.drawRect(0, qRound(height() * (1.0 - val_)) - selector_radius, width() - 1, selector_radius * 2);
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
  float t = clamp(static_cast<float>(i) / static_cast<float>(max), 0.0f, 1.0f);

  return Color(lerp(a.red(), b.red(), t),
               lerp(a.green(), b.green(), t),
               lerp(a.blue(), b.blue(), t));
}
