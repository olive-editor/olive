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

#include "keyframeviewitem.h"
#include "node/keyframe.h"
#include "widget/curvewidget/beziercontrolpointitem.h"
#include "widget/menu/menu.h"
#include "widget/timebased/timebasedview.h"
#include "widget/timetarget/timetarget.h"

namespace olive {

class KeyframeViewBase : public TimeBasedView, public TimeTargetObject
{
  Q_OBJECT
public:
  KeyframeViewBase(QWidget* parent = nullptr);

  virtual void Clear();

  void DeleteSelected();

  void AddKeyframesOfNode(Node* n);

  void AddKeyframesOfInput(Node *n, const QString &input);

  void AddKeyframesOfElement(const NodeInput &input);

  void AddKeyframesOfTrack(const NodeKeyframeTrackReference &ref);

  void RemoveKeyframesOfNode(Node* n);

  void RemoveKeyframesOfInput(Node *n, const QString &input);

  void RemoveKeyframesOfElement(const NodeInput &input);

  void RemoveKeyframesOfTrack(const NodeKeyframeTrackReference &ref);

  void SelectAll();

  void DeselectAll();

signals:
  void Dragged(int current_x, int current_y);

public slots:
  virtual KeyframeViewItem* AddKeyframe(NodeKeyframe* key);

  void RemoveKeyframe(NodeKeyframe* key);

protected:
  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;

  virtual void ScaleChangedEvent(const double& scale) override;

  const QMap<NodeKeyframe*, KeyframeViewItem*>& item_map() const;

  virtual void KeyframeAboutToBeRemoved(NodeKeyframe* key);

  virtual void TimeTargetChangedEvent(Node*) override;

  virtual void ContextMenuEvent(Menu &m);

  bool IsDragging() const
  {
    return dragging_;
  }

private:
  rational CalculateNewTimeFromScreen(const rational& old_time, double cursor_diff);

  static QPointF GenerateBezierControlPosition(const NodeKeyframe::BezierType mode,
                                               const QPointF& start_point,
                                               const QPointF& scaled_cursor_diff);

  QPointF GetScaledCursorPos(const QPointF &cursor_pos);

  struct KeyframeItemAndTime {
    KeyframeViewItem* key;
    qreal item_x;
    rational time;
    double value;
  };

  QMap<NodeKeyframe*, KeyframeViewItem*> item_map_;

  Tool::Item active_tool_;

  QPointF drag_start_;

  BezierControlPointItem* dragging_bezier_point_;
  QPointF dragging_bezier_point_start_;
  QPointF dragging_bezier_point_opposing_start_;

  KeyframeViewItem* initial_drag_item_;

  QVector<KeyframeItemAndTime> selected_keys_;

  bool currently_autoselecting_;

  bool dragging_;

private slots:
  void ShowContextMenu();

  void ShowKeyframePropertiesDialog();

  void AutoSelectKeyTimeNeighbors();

};

}

#endif // KEYFRAMEVIEWBASE_H
