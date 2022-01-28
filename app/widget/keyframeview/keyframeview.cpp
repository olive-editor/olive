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

#include "keyframeview.h"

#include <QMouseEvent>
#include <QToolTip>
#include <QVBoxLayout>

#include "common/qtutils.h"
#include "dialog/keyframeproperties/keyframeproperties.h"
#include "keyframeviewundo.h"
#include "node/node.h"
#include "widget/menu/menu.h"
#include "widget/menu/menushared.h"
#include "widget/nodeparamview/nodeparamviewundo.h"

namespace olive {

#define super TimeBasedView

KeyframeView::KeyframeView(QWidget *parent) :
  super(parent),
  selection_manager_(this),
  autoselect_siblings_(true),
  max_scroll_(0),
  first_chance_mouse_event_(false)
{
  setAlignment(Qt::AlignLeft | Qt::AlignTop);
  SetDefaultDragMode(RubberBandDrag);
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, &KeyframeView::customContextMenuRequested, this, &KeyframeView::ShowContextMenu);
}

void KeyframeView::DeleteSelected()
{
  MultiUndoCommand* command = new MultiUndoCommand();

  foreach (NodeKeyframe *key, GetSelectedKeyframes()) {
    command->add_child(new NodeParamRemoveKeyframeCommand(key));
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

KeyframeView::NodeConnections KeyframeView::AddKeyframesOfNode(Node *n)
{
  NodeConnections map;

  foreach (const QString& i, n->inputs()) {
    map.insert(i, AddKeyframesOfInput(n, i));
  }

  return map;
}

KeyframeView::InputConnections KeyframeView::AddKeyframesOfInput(Node* on, const QString& oinput)
{
  InputConnections vec;

  NodeInput resolved = NodeGroup::ResolveInput(NodeInput(on, oinput));
  Node *n = resolved.node();
  const QString &input = resolved.input();

  if (n->IsInputKeyframable(input)) {
    int arr_sz = n->InputArraySize(input);
    vec.resize(arr_sz + 1);
    for (int i=-1; i<arr_sz; i++) {
      vec[i+1] = AddKeyframesOfElement(NodeInput(n, input, i));
    }
  }

  return vec;
}

KeyframeView::ElementConnections KeyframeView::AddKeyframesOfElement(const NodeInput& input)
{
  const QVector<NodeKeyframeTrack>& tracks = input.node()->GetKeyframeTracks(input);
  ElementConnections vec(tracks.size());

  for (int i=0; i<tracks.size(); i++) {
    vec[i] = AddKeyframesOfTrack(NodeKeyframeTrackReference(input, i));
  }

  return vec;
}

KeyframeViewInputConnection *KeyframeView::AddKeyframesOfTrack(const NodeKeyframeTrackReference& ref)
{
  KeyframeViewInputConnection *track = new KeyframeViewInputConnection(ref, this);
  connect(track, &KeyframeViewInputConnection::RequireUpdate, this, &KeyframeView::Redraw);
  tracks_.append(track);
  Redraw();
  return track;
}

void KeyframeView::RemoveKeyframesOfTrack(KeyframeViewInputConnection *connection)
{
  if (tracks_.removeOne(connection)) {
    foreach (NodeKeyframe *key, connection->GetKeyframes()) {
      selection_manager_.Deselect(key);
    }
    delete connection;
    Redraw();
    emit SelectionChanged();
  }
}

void KeyframeView::SelectAll()
{
  foreach (KeyframeViewInputConnection *track, tracks_) {
    foreach (NodeKeyframe *key, track->GetKeyframes()) {
      SelectKeyframe(key);
    }
  }
}

void KeyframeView::DeselectAll()
{
  selection_manager_.ClearSelection();

  Redraw();
}

void KeyframeView::Clear()
{
  if (!tracks_.isEmpty()) {
    qDeleteAll(tracks_);
    tracks_.clear();
    Redraw();
  }

  selection_manager_.ClearSelection();
}

void KeyframeView::SelectionManagerSelectEvent(void *obj)
{
  if (autoselect_siblings_) {
    NodeKeyframe *key = static_cast<NodeKeyframe*>(obj);
    QVector<NodeKeyframe*> keys = key->parent()->GetKeyframesAtTime(key->input(), key->time(), key->element());
    foreach (NodeKeyframe* k, keys) {
      if (k != key) {
        SelectKeyframe(k);
      }
    }
  }

  emit SelectionChanged();
}

void KeyframeView::SelectionManagerDeselectEvent(void *obj)
{
  if (autoselect_siblings_) {
    NodeKeyframe *key = static_cast<NodeKeyframe*>(obj);
    QVector<NodeKeyframe*> keys = key->parent()->GetKeyframesAtTime(key->input(), key->time(), key->element());
    foreach (NodeKeyframe* k, keys) {
      if (k != key) {
        DeselectKeyframe(k);
      }
    }
  }

  emit SelectionChanged();
}

void KeyframeView::mousePressEvent(QMouseEvent *event)
{
  NodeKeyframe *key_under_cursor = selection_manager_.GetObjectAtPoint(event->pos());

  if (HandPress(event) || (!key_under_cursor && PlayheadPress(event))) {
    return;
  }

  // Do mouse press things
  if (FirstChanceMousePress(event)) {
    first_chance_mouse_event_ = true;
  } else if (NodeKeyframe *initial_key = selection_manager_.MousePress(event)) {
    selection_manager_.DragStart(initial_key, event);
    KeyframeDragStart(event);
  } else {
    selection_manager_.RubberBandStart(event);
  }

  // Update view
  Redraw();
}

void KeyframeView::mouseMoveEvent(QMouseEvent *event)
{
  if (HandMove(event) || PlayheadMove(event)) {
    return;
  }

  if (first_chance_mouse_event_) {
    FirstChanceMouseMove(event);
  } else if (selection_manager_.IsDragging()) {
    QString tip;
    KeyframeDragMove(event, tip);
    selection_manager_.DragMove(event, tip);
  } else if (selection_manager_.IsRubberBanding()) {
    selection_manager_.RubberBandMove(event);
    Redraw();
  }

  if (event->buttons()) {
    // Signal cursor pos in case we should scroll to catch up to it
    QPointF scene_pos = mapToScene(event->pos());
    emit Dragged(scene_pos.x(), scene_pos.y());
  }
}

void KeyframeView::mouseReleaseEvent(QMouseEvent *event)
{
  if (HandRelease(event) || PlayheadRelease(event)) {
    return;
  }

  if (first_chance_mouse_event_) {
    FirstChanceMouseRelease(event);
    first_chance_mouse_event_ = false;
  } else if (selection_manager_.IsDragging()) {
    MultiUndoCommand* command = new MultiUndoCommand();
    selection_manager_.DragStop(command);
    KeyframeDragRelease(event, command);
    Core::instance()->undo_stack()->push(command);
  } else if (selection_manager_.IsRubberBanding()) {
    selection_manager_.RubberBandStop();
    Redraw();
    emit SelectionChanged();
  }
}

int BinarySearchFirstKeyframeAfterOrAt(const QVector<NodeKeyframe*> &keys, const rational &time)
{
  int low = 0;
  int high = keys.size()-1;

  while (low <= high) {
    int mid = low + (high-low)/2;
    NodeKeyframe *test_key = keys.at(mid);

    if (test_key->time() == time || (test_key->time() > time && (mid == 0 || keys.at(mid-1)->time() < time))) {
      return mid;
    } else if (test_key->time() < time) {
      low = mid + 1;
    } else {
      high = mid - 1;
    }
  }

  return keys.size();
}

void KeyframeView::drawForeground(QPainter *painter, const QRectF &rect)
{
  int key_sz = QtUtils::QFontMetricsWidth(fontMetrics(), "Oi");
  int key_rad = key_sz/2;

  selection_manager_.ClearDrawnObjects();

  painter->setRenderHint(QPainter::Antialiasing);

  foreach (KeyframeViewInputConnection *track, tracks_) {
    const QVector<NodeKeyframe*> &keys = track->GetKeyframes();

    if (keys.isEmpty()) {
      continue;
    }

    if (!IsYAxisEnabled()) {
      // Filter out if the keyframes are offscreen Y
      qreal y = GetKeyframeSceneY(track, keys.first());
      if (y + key_rad < rect.top() || y - key_rad >= rect.bottom()) {
        continue;
      }
    }

    // Find first keyframe to show with binary search
    rational left_time = GetUnadjustedKeyframeTime(keys.first(), SceneToTime(rect.left() - key_sz));
    int using_index = BinarySearchFirstKeyframeAfterOrAt(keys, left_time);

    rational next_key = RATIONAL_MIN;
    NodeKeyframe::Type last_type = NodeKeyframe::kInvalid;
    for (int i=using_index; i<keys.size(); i++) {
      NodeKeyframe *key = keys.at(i);

      if (key->time() < next_key && key->type() == last_type) {
        // This key will be drawn at exactly the same location as the last one and therefore
        // doesn't need to be drawn. See if the next one will be drawn.
        i++;
        if (i == keys.size()) {
          break;
        }

        key = keys.at(i);

        if (key->time() < next_key) {
          // Next key still won't be drawn, so we'll switch to a binary search
          i = BinarySearchFirstKeyframeAfterOrAt(keys, next_key);

          if (i == keys.size()) {
            break;
          }

          key = keys.at(i);
        }
      }

      QRectF key_rect(-key_rad, -key_rad, key_sz, key_sz);
      qreal key_x = GetKeyframeSceneX(key);
      key_rect.translate(key_x, GetKeyframeSceneY(track, key));

      if (key_rect.left() >= rect.right()) {
        // Break after last keyframe
        break;
      }

      DrawKeyframe(painter, key, track, key_rect);

      next_key = GetUnadjustedKeyframeTime(key, SceneToTime(key_x + 1));
      last_type = key->type();
    }
  }

  super::drawForeground(painter, rect);
}

void KeyframeView::DrawKeyframe(QPainter *painter, NodeKeyframe *key, KeyframeViewInputConnection *track, const QRectF &key_rect)
{
  painter->setPen(Qt::black);

  if (IsKeyframeSelected(key)) {
    painter->setBrush(palette().highlight());
  } else {
    painter->setBrush(track->GetBrush());
  }

  selection_manager_.DeclareDrawnObject(key, key_rect);

  switch (key->type()) {
  case NodeKeyframe::kInvalid:
    break;
  case NodeKeyframe::kLinear:
  {
    QPointF points[] = {
      QPointF(key_rect.center().x(), key_rect.top()),
      QPointF(key_rect.right(), key_rect.center().y()),
      QPointF(key_rect.center().x(), key_rect.bottom()),
      QPointF(key_rect.left(), key_rect.center().y())
    };

    painter->drawPolygon(points, 4);
    break;
  }
  case NodeKeyframe::kBezier:
    painter->drawEllipse(key_rect);
    break;
  case NodeKeyframe::kHold:
    painter->drawRect(key_rect);
    break;
  }
}

void KeyframeView::ScaleChangedEvent(const double &scale)
{
  super::ScaleChangedEvent(scale);

  Redraw();
}

void KeyframeView::TimeTargetChangedEvent(Node *target)
{
  Redraw();
}

void KeyframeView::TimebaseChangedEvent(const rational &timebase)
{
  super::TimebaseChangedEvent(timebase);

  selection_manager_.SetTimebase(timebase);
}

void KeyframeView::ContextMenuEvent(Menu& m)
{
  Q_UNUSED(m)
}

void KeyframeView::SelectKeyframe(NodeKeyframe *key)
{
  if (selection_manager_.Select(key)) {
    Redraw();

    emit SelectionChanged();
  }
}

void KeyframeView::DeselectKeyframe(NodeKeyframe *key)
{
  if (selection_manager_.Deselect(key)) {
    Redraw();

    emit SelectionChanged();
  }
}

rational KeyframeView::GetUnadjustedKeyframeTime(NodeKeyframe *key, const rational &time)
{
  return GetAdjustedTime(GetTimeTarget(), key->parent(), time, true);
}

rational KeyframeView::GetAdjustedKeyframeTime(NodeKeyframe *key)
{
  return GetAdjustedTime(key->parent(), GetTimeTarget(), key->time(), false);
}

double KeyframeView::GetKeyframeSceneX(NodeKeyframe *key)
{
  return TimeToScene(GetAdjustedKeyframeTime(key));
}

qreal KeyframeView::GetKeyframeSceneY(KeyframeViewInputConnection *track, NodeKeyframe *key)
{
  return mapFromGlobal(QPoint(0, track->GetKeyframeY())).y();
}

void KeyframeView::SceneRectUpdateEvent(QRectF &rect)
{
  rect.setY(0);
  rect.setHeight(max_scroll_);
}

rational KeyframeView::CalculateNewTimeFromScreen(const rational &old_time, double cursor_diff)
{
  return rational::fromDouble(old_time.toDouble() + cursor_diff);
}

void KeyframeView::ShowContextMenu()
{
  Menu m;

  MenuShared::instance()->AddItemsForEditMenu(&m, false);

  QAction* linear_key_action = nullptr;
  QAction* bezier_key_action = nullptr;
  QAction* hold_key_action = nullptr;

  if (!GetSelectedKeyframes().isEmpty()) {
    bool all_keys_are_same_type = true;
    NodeKeyframe::Type type = GetSelectedKeyframes().first()->type();

    for (int i=1;i<GetSelectedKeyframes().size();i++) {
      NodeKeyframe* key_item = GetSelectedKeyframes().at(i);
      NodeKeyframe* prev_item = GetSelectedKeyframes().at(i-1);

      if (key_item->type() != prev_item->type()) {
        all_keys_are_same_type = false;
        break;
      }
    }

    m.addSeparator();

    linear_key_action = m.addAction(tr("Linear"));
    bezier_key_action = m.addAction(tr("Bezier"));
    hold_key_action = m.addAction(tr("Hold"));

    if (all_keys_are_same_type) {
      switch (type) {
      case NodeKeyframe::kInvalid:
        break;
      case NodeKeyframe::kLinear:
        linear_key_action->setChecked(true);
        break;
      case NodeKeyframe::kBezier:
        bezier_key_action->setChecked(true);
        break;
      case NodeKeyframe::kHold:
        hold_key_action->setChecked(true);
        break;
      }
    }
  }

  m.addSeparator();

  AddSetScrollZoomsByDefaultActionToMenu(&m);

  m.addSeparator();

  ContextMenuEvent(m);

  if (!GetSelectedKeyframes().isEmpty()) {
    m.addSeparator();

    QAction* properties_action = m.addAction(tr("P&roperties"));
    connect(properties_action, &QAction::triggered, this, &KeyframeView::ShowKeyframePropertiesDialog);
  }

  QAction* selected = m.exec(QCursor::pos());

  // Process keyframe type changes
  if (selected) {
    if (selected == linear_key_action
        || selected == bezier_key_action
        || selected == hold_key_action) {
      NodeKeyframe::Type new_type;

      if (selected == hold_key_action) {
        new_type = NodeKeyframe::kHold;
      } else if (selected == bezier_key_action) {
        new_type = NodeKeyframe::kBezier;
      } else {
        new_type = NodeKeyframe::kLinear;
      }

      MultiUndoCommand* command = new MultiUndoCommand();
      foreach (NodeKeyframe* item, GetSelectedKeyframes()) {
        command->add_child(new KeyframeSetTypeCommand(item, new_type));
      }
      Core::instance()->undo_stack()->push(command);
    }
  }
}

void KeyframeView::ShowKeyframePropertiesDialog()
{
  if (!GetSelectedKeyframes().isEmpty()) {
    KeyframePropertiesDialog kd(GetSelectedKeyframes(), timebase(), this);
    kd.exec();
  }
}

void KeyframeView::Redraw()
{
  viewport()->update();
}

}
