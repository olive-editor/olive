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

#ifndef RESIZABLESCROLLBAR_H
#define RESIZABLESCROLLBAR_H

#include <QScrollBar>

#include "common/define.h"

namespace olive {

class ResizableScrollBar : public QScrollBar
{
  Q_OBJECT
public:
  ResizableScrollBar(QWidget* parent = nullptr);
  ResizableScrollBar(Qt::Orientation orientation, QWidget* parent = nullptr);

signals:
  void ResizeBegan(int old_bar_width, bool top_handle);

  void ResizeMoved(int movement);

  void ResizeEnded();

protected:
  virtual void mousePressEvent(QMouseEvent* event) override;

  virtual void mouseMoveEvent(QMouseEvent* event) override;

  virtual void mouseReleaseEvent(QMouseEvent* event) override;

private:
  QRect GetScrollBarRect();

  static const int kHandleWidth;

  enum MouseHandleState {
    kNotInHandle,
    kInTopHandle,
    kInBottomHandle
  };

  void Init();

  int GetActiveMousePos(QMouseEvent* event);

  int GetActiveBarSize();

  MouseHandleState mouse_handle_state_;

  bool dragging_;

  int drag_start_point_;

};

}

#endif // RESIZABLESCROLLBAR_H
