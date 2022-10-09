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

#include "sliderlabel.h"

#include <QApplication>
#include <QMouseEvent>
#include <QDebug>

namespace olive {

SliderLabel::SliderLabel(QWidget *parent) :
  QLabel(parent),
  override_color_enabled_(false)
{
  QPalette p = palette();

  p.setColor(QPalette::Disabled,
             QPalette::Highlight,
             p.color(QPalette::Disabled, QPalette::ButtonText));

  setPalette(p);

  // Use highlight color as font color
  setForegroundRole(QPalette::Link);

  // Set underlined
  QFont f = font();
  f.setUnderline(true);
  setFont(f);

  // Allow users to tab to this widget
  setFocusPolicy(Qt::TabFocus);

  // Add custom context menu
  setContextMenuPolicy(Qt::CustomContextMenu);
}

void SliderLabel::SetColor(const QColor &c)
{
  // Prevent infinite loop in changeEvent when we set the stylesheet
  override_color_enabled_ = false;
  override_color_ = c;

  // Different colors will look different depending on the theme (light/dark mode). We abstract
  // that away here so that other classes can simply choose a color and we will handle making it
  // more legible based on the background
  QColor adjusted;
  if (palette().window().color().lightness() < 128) {
    adjusted = override_color_.lighter(150);
  } else {
    adjusted = override_color_.darker(150);
  }

  setStyleSheet(QStringLiteral("color: %1").arg(adjusted.name()));
  override_color_enabled_ = true;
}

void SliderLabel::mousePressEvent(QMouseEvent *e)
{
  if (e->button() == Qt::LeftButton) {
    if (e->modifiers() & Qt::AltModifier) {
      emit RequestReset();
    } else {
      emit LabelPressed();
    }
  }
}

void SliderLabel::mouseReleaseEvent(QMouseEvent *e)
{
  if (e->button() == Qt::LeftButton) {
    if (!(e->modifiers() & Qt::AltModifier)) {
      emit LabelReleased();
    }
  }
}

void SliderLabel::focusInEvent(QFocusEvent *event)
{
  QWidget::focusInEvent(event);

  if (event->reason() == Qt::TabFocusReason) {
    emit focused();
  }
}

void SliderLabel::changeEvent(QEvent *event)
{
  QWidget::changeEvent(event);

  if (override_color_enabled_ && event->type() == QEvent::StyleChange) {
    SetColor(override_color_);
  }
}

}
