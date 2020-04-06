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

#ifndef KEYFRAMEVIEWBASE_H
#define KEYFRAMEVIEWBASE_H

#include "keyframeviewitem.h"
#include "node/keyframe.h"
#include "timetargetobject.h"
#include "widget/curvewidget/beziercontrolpointitem.h"
#include "widget/timelinewidget/view/timelineviewbase.h"

OLIVE_NAMESPACE_ENTER

class KeyframeViewBase : public TimelineViewBase, public TimeTargetObject
{
  Q_OBJECT
public:
  KeyframeViewBase(QWidget* parent = nullptr);

  virtual void Clear();

  const double& GetYScale() const;
  void SetYScale(const double& y_scale);

public slots:
  void RemoveKeyframe(NodeKeyframePtr key);

protected:
  virtual KeyframeViewItem* AddKeyframeInternal(NodeKeyframePtr key);

  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;

  virtual void ScaleChangedEvent(const double& scale) override;

  virtual void VerticalScaleChangedEvent(double scale);

  const QMap<NodeKeyframe*, KeyframeViewItem*>& item_map() const;

  virtual void KeyframeAboutToBeRemoved(NodeKeyframe* key);

  virtual void TimeTargetChangedEvent(Node*) override;

  void SetYAxisEnabled(bool e);

private:
  rational CalculateNewTimeFromScreen(const rational& old_time, double cursor_diff);

  static QPointF GenerateBezierControlPosition(const NodeKeyframe::BezierType mode,
                                               const QPointF& start_point,
                                               const QPointF& scaled_cursor_diff);

  void ProcessBezierDrag(QPointF mouse_diff_scaled, bool include_opposing, bool undoable);

  QPointF GetScaledCursorPos(const QPoint& cursor_pos);

  struct KeyframeItemAndTime {
    KeyframeViewItem* key;
    qreal item_x;
    rational time;
    double value;
  };

  QMap<NodeKeyframe*, KeyframeViewItem*> item_map_;

  Tool::Item active_tool_;

  QPoint drag_start_;

  BezierControlPointItem* dragging_bezier_point_;
  QPointF dragging_bezier_point_start_;
  QPointF dragging_bezier_point_opposing_start_;

  QVector<KeyframeItemAndTime> selected_keys_;

  bool y_axis_enabled_;

  double y_scale_;

private slots:
  void ShowContextMenu();

  void ShowKeyframePropertiesDialog();

};

OLIVE_NAMESPACE_EXIT

#endif // KEYFRAMEVIEWBASE_H
