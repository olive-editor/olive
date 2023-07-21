/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include "dragbutton.h"

#include <QMouseEvent>

namespace olive {

DragButton::DragButton(QWidget *parent) :
  QPushButton(parent)
{
  setCursor(Qt::OpenHandCursor);
  dragging_ = false;
}

void DragButton::mousePressEvent(QMouseEvent *event)
{
  QPushButton::mousePressEvent(event);
}

void DragButton::mouseMoveEvent(QMouseEvent *event)
{
  QPushButton::mouseMoveEvent(event);

  if (event->buttons() && !dragging_) {
    emit DragStarted();
    dragging_ = true;
  }
}

void DragButton::mouseReleaseEvent(QMouseEvent *event)
{
  QPushButton::mouseReleaseEvent(event);

  dragging_ = false;
}

}
