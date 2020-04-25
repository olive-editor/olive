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

#ifndef TIMELINEVIEWBASE_H
#define TIMELINEVIEWBASE_H

#include <QGraphicsView>

#include "core.h"
#include "timelineplayhead.h"
#include "widget/timelinewidget/timelinescaledobject.h"

OLIVE_NAMESPACE_ENTER

class TimelineViewBase : public QGraphicsView, public TimelineScaledObject
{
  Q_OBJECT
public:
  TimelineViewBase(QWidget* parent = nullptr);

  static const double kMaximumScale;

public slots:
  void SetTime(const int64_t time);

  void SetEndTime(const rational& length);

signals:
  void TimeChanged(const int64_t& time);

  void ScaleChanged(double scale);

  void RequestCenterScrollOnPlayhead();

protected:
  virtual void drawForeground(QPainter *painter, const QRectF &rect) override;

  virtual void resizeEvent(QResizeEvent *event) override;

  virtual void ScaleChangedEvent(const double& scale) override;

  virtual void SceneRectUpdateEvent(QRectF&){}

  bool HandleZoomFromScroll(QWheelEvent* event);

  bool WheelEventIsAZoomEvent(QWheelEvent* event);

  void SetLimitYAxis(bool e);

  rational GetPlayheadTime() const;

  void SetDefaultDragMode(DragMode mode);
  const DragMode& GetDefaultDragMode() const;

  bool PlayheadPress(QMouseEvent* event);
  bool PlayheadMove(QMouseEvent* event);
  bool PlayheadRelease(QMouseEvent* event);

  bool HandPress(QMouseEvent* event);
  bool HandMove(QMouseEvent* event);
  bool HandRelease(QMouseEvent* event);

  virtual void ToolChangedEvent(Tool::Item tool);

  virtual void TimebaseChangedEvent(const rational &) override;

private:
  qreal GetPlayheadX();

  int64_t playhead_;

  TimelinePlayhead playhead_style_;

  double playhead_scene_left_;
  double playhead_scene_right_;

  bool dragging_playhead_;

  bool dragging_hand_;
  DragMode pre_hand_drag_mode_;

  QGraphicsScene scene_;

  bool limit_y_axis_;

  DragMode default_drag_mode_;

  rational end_time_;

private slots:
  /**
   * @brief Slot called whenever the view resizes or the scene contents change to enforce minimum scene sizes
   */
  void UpdateSceneRect();

  /**
   * @brief Slot to handle page scrolling of the playhead
   *
   * If the playhead is outside the current scroll bounds, this function will scroll to where it is. Otherwise it will
   * do nothing.
   */
  void PageScrollToPlayhead();

  void ApplicationToolChanged(Tool::Item tool);

};

OLIVE_NAMESPACE_EXIT

#endif // TIMELINEVIEWBASE_H
