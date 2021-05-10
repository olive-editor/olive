/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef HANDMOVABLEVIEW_H
#define HANDMOVABLEVIEW_H

#include <QGraphicsView>
#include <QMenu>

#include "tool/tool.h"

namespace olive {

class HandMovableView : public QGraphicsView
{
  Q_OBJECT
public:
  HandMovableView(QWidget* parent = nullptr);

  bool GetScrollZoomsByDefault() const
  {
    return scroll_zooms_by_default_;
  }

  QAction* AddSetScrollZoomsByDefaultActionToMenu(QMenu* menu, bool autoconnect = true);

public slots:
  void SetScrollZoomsByDefault(bool e)
  {
    scroll_zooms_by_default_ = e;
  }

protected:
  virtual void ToolChangedEvent(Tool::Item tool){Q_UNUSED(tool)}

  bool HandPress(QMouseEvent* event);
  bool HandMove(QMouseEvent* event);
  bool HandRelease(QMouseEvent* event);

  void SetDefaultDragMode(DragMode mode);
  const DragMode& GetDefaultDragMode() const;

  bool WheelEventIsAZoomEvent(QWheelEvent* event) const;

  virtual void wheelEvent(QWheelEvent* event) override;

  virtual void ZoomIntoCursorPosition(QWheelEvent* event, double multiplier, const QPointF &cursor_pos);

private:
  bool dragging_hand_;
  DragMode pre_hand_drag_mode_;

  DragMode default_drag_mode_;

  /**
   * @brief Whether scrolling should perform a scroll or a zoom
   *
   * If TRUE, scrolling will ZOOM and Ctrl+Scroll with SCROLL.
   * If FALSE (default), scrolling will SCROLL and Ctrl+Scroll will ZOOM.
   */
  bool scroll_zooms_by_default_;

private slots:
  void ApplicationToolChanged(Tool::Item tool);

};

}

#endif // HANDMOVABLEVIEW_H
