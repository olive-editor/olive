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
#include "widget/menu/menu.h"
#include "widget/timebased/timebasedview.h"
#include "widget/timebased/timebasedviewselectionmanager.h"
#include "widget/timetarget/timetarget.h"

namespace olive {

class KeyframeView : public TimeBasedView, public TimeTargetObject
{
  Q_OBJECT
public:
  KeyframeView(QWidget* parent = nullptr);

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

  virtual void SelectionManagerSelectEvent(void *obj) override;
  virtual void SelectionManagerDeselectEvent(void *obj) override;

  void SetMaxScroll(int i)
  {
    max_scroll_ = i;
    UpdateSceneRect();
  }

signals:
  void Dragged(int current_x, int current_y);

  void SelectionChanged();

protected:
  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;

  virtual void drawForeground(QPainter *painter, const QRectF &rect) override;

  virtual void DrawKeyframe(QPainter *painter, NodeKeyframe *key, KeyframeViewInputConnection *track, const QRectF &key_rect);

  virtual void ScaleChangedEvent(const double& scale) override;

  virtual void TimeTargetChangedEvent(Node*) override;

  virtual void TimebaseChangedEvent(const rational &timebase) override;

  virtual void ContextMenuEvent(Menu &m);

  virtual bool FirstChanceMousePress(QMouseEvent *event){return false;}
  virtual void FirstChanceMouseMove(QMouseEvent *event){}
  virtual void FirstChanceMouseRelease(QMouseEvent *event){}

  virtual void KeyframeDragStart(QMouseEvent *event){}
  virtual void KeyframeDragMove(QMouseEvent *event, QString &tip){}
  virtual void KeyframeDragRelease(QMouseEvent *event, MultiUndoCommand *command){}

  void SelectKeyframe(NodeKeyframe *key);

  void DeselectKeyframe(NodeKeyframe *key);

  bool IsKeyframeSelected(NodeKeyframe *key) const
  {
    return selection_manager_.IsSelected(key);
  }

  rational GetUnadjustedKeyframeTime(NodeKeyframe *key, const rational &time);
  rational GetUnadjustedKeyframeTime(NodeKeyframe *key)
  {
    return GetUnadjustedKeyframeTime(key, key->time());
  }

  rational GetAdjustedKeyframeTime(NodeKeyframe *key);

  double GetKeyframeSceneX(NodeKeyframe *key);

  virtual qreal GetKeyframeSceneY(KeyframeViewInputConnection *track, NodeKeyframe *key);

  void SetAutoSelectSiblings(bool e)
  {
    autoselect_siblings_ = e;
  }

  virtual void SceneRectUpdateEvent(QRectF& rect) override;

protected slots:
  void Redraw();

private:
  rational CalculateNewTimeFromScreen(const rational& old_time, double cursor_diff);

  QVector<KeyframeViewInputConnection*> tracks_;

  TimeBasedViewSelectionManager<NodeKeyframe> selection_manager_;

  bool autoselect_siblings_;

  int max_scroll_;

  bool first_chance_mouse_event_;

private slots:
  void ShowContextMenu();

  void ShowKeyframePropertiesDialog();

};

}

#endif // KEYFRAMEVIEWBASE_H
