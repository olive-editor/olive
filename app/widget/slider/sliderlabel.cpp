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

SliderLabel::SliderLabel(QWidget *parent) :
  QLabel(parent)
{
  // Use highlight color as font color
  setForegroundRole(QPalette::Highlight);

  // Set underlined
  QFont f = font();
  f.setUnderline(true);
  setFont(f);
}

void SliderLabel::mousePressEvent(QMouseEvent *ev)
{
  QLabel::mousePressEvent(ev);

  drag_start_ = QCursor::pos();

  static_cast<QGuiApplication*>(QApplication::instance())->setOverrideCursor(Qt::BlankCursor);

  emit drag_start();
}

void SliderLabel::mouseMoveEvent(QMouseEvent *ev)
{
  QLabel::mouseMoveEvent(ev);

  QPoint current_pos = QCursor::pos();

  int x_mvmt = current_pos.x() - drag_start_.x();
  int y_mvmt = drag_start_.y() - current_pos.y();

  emit dragged(x_mvmt + y_mvmt);

  // Keep cursor in the same position
  QCursor::setPos(drag_start_);
}

void SliderLabel::mouseReleaseEvent(QMouseEvent *ev)
{
  QWidget::mouseReleaseEvent(ev);

  // Emit a clicked signal
  emit drag_stop();

  static_cast<QGuiApplication*>(QApplication::instance())->restoreOverrideCursor();
}
