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
  QLabel(parent)
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

void SliderLabel::mousePressEvent(QMouseEvent *)
{
  if (QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier)) {
    emit ResetResult();
  }else {
    emit drag_start();

#if defined(Q_OS_MAC)
    CGAssociateMouseAndMouseCursorPosition(false);
    CGDisplayHideCursor(kCGDirectMainDisplay);
    CGGetLastMouseDelta(nullptr, nullptr);
#else
    drag_start_ = QCursor::pos();

    static_cast<QGuiApplication*>(QApplication::instance())->setOverrideCursor(Qt::BlankCursor);
#endif
  }
}

void SliderLabel::mouseMoveEvent(QMouseEvent *)
{
  if (QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier)) {
      // do nothing
  }else {
    int32_t x_mvmt, y_mvmt;

    // Keep cursor in the same position
#if defined(Q_OS_MAC)
    CGGetLastMouseDelta(&x_mvmt, &y_mvmt);
#else
    QPoint current_pos = QCursor::pos();

    x_mvmt = current_pos.x() - drag_start_.x();
    y_mvmt = drag_start_.y() - current_pos.y();

    QCursor::setPos(drag_start_);
#endif

    emit dragged(x_mvmt + y_mvmt);
  }
}

void SliderLabel::mouseReleaseEvent(QMouseEvent *)
{
  if (QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier)) {
    //do nothing 
  } else {
    // Emit a clicked signal
    emit drag_stop();
  }
#if defined(Q_OS_MAC)
  CGAssociateMouseAndMouseCursorPosition(true);
  CGDisplayShowCursor(kCGDirectMainDisplay);
#else
  static_cast<QGuiApplication*>(QApplication::instance())->restoreOverrideCursor();
#endif
}

void SliderLabel::focusInEvent(QFocusEvent *event)
{
  QWidget::focusInEvent(event);

  if (event->reason() == Qt::TabFocusReason) {
    emit focused();
  }
}

OLIVE_NAMESPACE_EXIT
