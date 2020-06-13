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

#include "sliderlabel.h"

#include <QApplication>
#include <QMouseEvent>
#include <QDebug>

#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

OLIVE_NAMESPACE_ENTER

SliderLabel::SliderLabel(QWidget *parent) :
  QLabel(parent),
  dragging_(false)
{
  QPalette p = palette();

  p.setColor(QPalette::Disabled,
             QPalette::Highlight,
             p.color(QPalette::Disabled, QPalette::ButtonText));

  setPalette(p);

  // Use highlight color as font color
  setForegroundRole(QPalette::Highlight);

  // Set underlined
  QFont f = font();
  f.setUnderline(true);
  setFont(f);

  // Allow users to tab to this widget
  setFocusPolicy(Qt::TabFocus);
}

void SliderLabel::mousePressEvent(QMouseEvent *e)
{
  if (e->modifiers() & Qt::AltModifier) {
    emit RequestReset();
  } else {
    dragging_ = true;
    emit LabelPressed();
  }
}

void SliderLabel::mouseMoveEvent(QMouseEvent *)
{
  if (dragging_) {
    emit LabelMoved();
  }
}

void SliderLabel::mouseReleaseEvent(QMouseEvent *)
{
  if (dragging_) {
    emit LabelReleased();
    dragging_ = false;
  }
}

void SliderLabel::focusInEvent(QFocusEvent *event)
{
  QWidget::focusInEvent(event);

  if (event->reason() == Qt::TabFocusReason) {
    emit focused();
  }
}

OLIVE_NAMESPACE_EXIT
