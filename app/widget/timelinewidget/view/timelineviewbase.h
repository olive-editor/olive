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

#ifndef TIMELINEVIEWBASE_H
#define TIMELINEVIEWBASE_H

#include <QGraphicsView>

#include "core.h"
#include "handmovableview.h"
#include "widget/timelinewidget/snapservice.h"
#include "widget/timelinewidget/timelinescaledobject.h"

namespace olive {

class TimelineViewBase : public HandMovableView, public TimelineScaledObject
{
  Q_OBJECT
public:
  TimelineViewBase(QWidget* parent = nullptr);

  static const double kMaximumScale;

  void EnableSnap(const QList<rational>& points);
  void DisableSnap();
  bool IsSnapped() const
  {
    return snapped_;
  }

  void SetSnapService(SnapService* service);

  const double& GetYScale() const;
  void SetYScale(const double& y_scale);

public slots:
  void SetTime(const int64_t time);

  void SetEndTime(const rational& length);

signals:
  void TimeChanged(const int64_t& time);

  void ScaleChanged(double scale);

protected:
  virtual void drawForeground(QPainter *painter, const QRectF &rect) override;

  virtual void resizeEvent(QResizeEvent *event) override;

  virtual void ScaleChangedEvent(const double& scale) override;

  virtual void SceneRectUpdateEvent(QRectF&){}

  virtual void VerticalScaleChangedEvent(double scale);

  bool HandleZoomFromScroll(QWheelEvent* event);

  bool WheelEventIsAZoomEvent(QWheelEvent* event);

  rational GetPlayheadTime() const;

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

  int64_t playhead_;

  double playhead_scene_left_;
  double playhead_scene_right_;

  bool dragging_playhead_;

  QGraphicsScene scene_;

  bool snapped_;
  QList<rational> snap_time_;

  rational end_time_;

  SnapService* snap_service_;

  bool y_axis_enabled_;

  double y_scale_;

private slots:
  /**
   * @brief Slot called whenever the view resizes or the scene contents change to enforce minimum scene sizes
   */
  void UpdateSceneRect();

};

}

#endif // TIMELINEVIEWBASE_H
