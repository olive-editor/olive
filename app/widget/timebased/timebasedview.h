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

#ifndef TIMELINEVIEWBASE_H
#define TIMELINEVIEWBASE_H

#include <QGraphicsView>
#include <vector>

#include "core.h"
#include "timescaledobject.h"
#include "widget/handmovableview/handmovableview.h"

namespace olive {

class TimeBasedWidget;

class TimeBasedView : public HandMovableView, public TimeScaledObject
{
  Q_OBJECT
public:
  TimeBasedView(QWidget* parent = nullptr);

  void EnableSnap(const std::vector<rational> &points);
  void DisableSnap();
  bool IsSnapped() const
  {
    return snapped_;
  }

  TimeBasedWidget *GetSnapService() const { return snap_service_; }
  void SetSnapService(TimeBasedWidget* service) { snap_service_ = service; }

  const double& GetYScale() const;
  void SetYScale(const double& y_scale);

  virtual bool IsDraggingPlayhead() const
  {
    return dragging_playhead_;
  }

  // To be called only by selection managers
  virtual void SelectionManagerSelectEvent(void *obj){}
  virtual void SelectionManagerDeselectEvent(void *obj){}

  ViewerOutput *GetViewerNode() const { return viewer_; }

  void SetViewerNode(ViewerOutput *v);

public slots:
  void SetEndTime(const rational& length);

  /**
   * @brief Slot called whenever the view resizes or the scene contents change to enforce minimum scene sizes
   */
  void UpdateSceneRect();

signals:
  void ScaleChanged(double scale);

protected:
  virtual void drawForeground(QPainter *painter, const QRectF &rect) override;

  virtual void resizeEvent(QResizeEvent *event) override;

  virtual void ScaleChangedEvent(const double& scale) override;

  virtual void SceneRectUpdateEvent(QRectF&){}

  virtual void VerticalScaleChangedEvent(double scale);

  virtual void ZoomIntoCursorPosition(QWheelEvent *event, double multiplier, const QPointF &cursor_pos) override;

  bool PlayheadPress(QMouseEvent* event);
  bool PlayheadMove(QMouseEvent* event);
  bool PlayheadRelease(QMouseEvent* event);

  virtual void TimebaseChangedEvent(const rational &) override;

  bool IsYAxisEnabled() const
  {
    return y_axis_enabled_;
  }

  void SetYAxisEnabled(bool e)
  {
    y_axis_enabled_ = e;
  }

private:
  qreal GetPlayheadX();

  double playhead_scene_left_;
  double playhead_scene_right_;

  bool dragging_playhead_;

  QGraphicsScene scene_;

  bool snapped_;
  std::vector<rational> snap_time_;

  rational end_time_;

  TimeBasedWidget* snap_service_;

  bool y_axis_enabled_;

  double y_scale_;

  ViewerOutput *viewer_;

};

}

#endif // TIMELINEVIEWBASE_H
