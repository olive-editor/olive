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

#ifndef KEYFRAMEVIEWBASE_H
#define KEYFRAMEVIEWBASE_H

#include "keyframeviewinputconnection.h"
#include "node/keyframe.h"
#include "widget/curvewidget/beziercontrolpointitem.h"
#include "widget/menu/menu.h"
#include "widget/timebased/timebasedview.h"
#include "widget/timebased/timebasedviewselectionmanager.h"
#include "widget/timetarget/timetarget.h"

namespace olive {

class KeyframeViewBase : public TimeBasedView, public TimeTargetObject
{
  Q_OBJECT
public:
  KeyframeViewBase(QWidget* parent = nullptr);

  void DeleteSelected();

  using ElementConnections = QVector<KeyframeViewInputConnection *>;
  using InputConnections = QVector<ElementConnections>;
  using NodeConnections = QMap<QString, InputConnections>;

  NodeConnections AddKeyframesOfNode(Node* n);

  InputConnections AddKeyframesOfInput(Node *n, const QString &input);

  ElementConnections AddKeyframesOfElement(const NodeInput &input);

  KeyframeViewInputConnection *AddKeyframesOfTrack(const NodeKeyframeTrackReference &ref);

  void RemoveKeyframesOfTrack(KeyframeViewInputConnection *connection);

  void SelectAll();

  void DeselectAll();

  void Clear();

  const QVector<NodeKeyframe*> &GetSelectedKeyframes() const
  {
    return selection_manager_.GetSelectedObjects();
  }

signals:
  void Dragged(int current_x, int current_y);

protected:
  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;

  virtual void drawForeground(QPainter *painter, const QRectF &rect) override;

  virtual void ScaleChangedEvent(const double& scale) override;

  virtual void TimeTargetChangedEvent(Node*) override;

  virtual void TimebaseChangedEvent(const rational &timebase) override;

  virtual void ContextMenuEvent(Menu &m);

  bool IsDragging() const
  {
    return dragging_;
  }

  void SelectKeyframe(NodeKeyframe *key);

  void DeselectKeyframe(NodeKeyframe *key);

  bool IsKeyframeSelected(NodeKeyframe *key) const
  {
    return selection_manager_.IsSelected(key);
  }

  rational GetAdjustedKeyframeTime(NodeKeyframe *key);

  double GetKeyframeSceneX(NodeKeyframe *key);

private:
  rational CalculateNewTimeFromScreen(const rational& old_time, double cursor_diff);

  static QPointF GenerateBezierControlPosition(const NodeKeyframe::BezierType mode,
                                               const QPointF& start_point,
                                               const QPointF& scaled_cursor_diff);

  QPointF GetScaledCursorPos(const QPointF &cursor_pos);

  QPointF drag_start_;

  BezierControlPointItem* dragging_bezier_point_;
  QPointF dragging_bezier_point_start_;
  QPointF dragging_bezier_point_opposing_start_;

  QVector<KeyframeViewInputConnection*> tracks_;

  bool currently_autoselecting_;

  bool dragging_;

  TimeBasedViewSelectionManager<NodeKeyframe> selection_manager_;

private slots:
  void ShowContextMenu();

  void ShowKeyframePropertiesDialog();

  void AutoSelectKeyTimeNeighbors();

  void Redraw();

};

}

#endif // KEYFRAMEVIEWBASE_H
