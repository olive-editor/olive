/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "resizablescrollbar.h"

#include <QDebug>
#include <QMouseEvent>
#include <QStyle>
#include <QStyleOptionSlider>

#include "common/range.h"

namespace olive {

const int ResizableScrollBar::kHandleWidth = 10;

ResizableScrollBar::ResizableScrollBar(QWidget *parent) :
  QScrollBar(parent)
{
  Init();
}

ResizableScrollBar::ResizableScrollBar(Qt::Orientation orientation, QWidget *parent):
  QScrollBar(orientation, parent)
{
  Init();
}

void ResizableScrollBar::mousePressEvent(QMouseEvent *event)
{
  if (mouse_handle_state_ == kNotInHandle) {
    QScrollBar::mousePressEvent(event);
  } else {
    dragging_ = true;

    drag_start_point_ = GetActiveMousePos(event);

    emit ResizeBegan(GetActiveBarSize(), (mouse_handle_state_ == kInTopHandle));
  }
}

void ResizableScrollBar::mouseMoveEvent(QMouseEvent *event)
{
  QRect sr = GetScrollBarRect();

  if (dragging_) {
    // Determine how much the cursor has moved
    int mouse_movement = GetActiveMousePos(event) - drag_start_point_;

    emit ResizeMoved(mouse_movement);

  } else {

    int mouse_pos, top, bottom;
    Qt::CursorShape target_cursor;
    mouse_pos = GetActiveMousePos(event);

    if (orientation() == Qt::Horizontal) {
      top = sr.left();
      bottom = sr.right();
      target_cursor = Qt::SizeHorCursor;
    } else {
      top = sr.top();
      bottom = sr.bottom();
      target_cursor = Qt::SizeVerCursor;
    }

    if (InRange(mouse_pos, top, kHandleWidth)) {
      mouse_handle_state_ = kInTopHandle;
    } else if (InRange(mouse_pos, bottom, kHandleWidth)) {
      mouse_handle_state_ = kInBottomHandle;
    } else {
      mouse_handle_state_ = kNotInHandle;
    }

    if (mouse_handle_state_ == kNotInHandle) {
      unsetCursor();
    } else {
      setCursor(target_cursor);
    }

    QScrollBar::mouseMoveEvent(event);

  }
}

void ResizableScrollBar::mouseReleaseEvent(QMouseEvent *event)
{
  if (dragging_) {
    dragging_ = false;

    emit ResizeEnded();
  } else {
    QScrollBar::mouseReleaseEvent(event);
  }
}

QRect ResizableScrollBar::GetScrollBarRect()
{
  // Initialize "style option". I don't know what this does, I just ripped it straight from
  // Qt source code
  QStyleOptionSlider opt;
  initStyleOption(&opt);

  // Determine rect of slider bar
  return style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                 QStyle::SC_ScrollBarSlider, this);
}

void ResizableScrollBar::Init()
{
  setSingleStep(20);
  setMaximum(0);
  setMouseTracking(true);

  mouse_handle_state_= kNotInHandle;
  dragging_ = false;
}

int ResizableScrollBar::GetActiveMousePos(QMouseEvent *event)
{
  if (orientation() == Qt::Horizontal) {
    return event->pos().x();
  } else {
    return event->pos().y();
  }
}

int ResizableScrollBar::GetActiveBarSize()
{
  QRect sr = GetScrollBarRect();

  if (orientation() == Qt::Horizontal) {
    return sr.width();
  } else {
    return sr.height();
  }
}

}
