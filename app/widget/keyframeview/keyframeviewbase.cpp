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

#include "keyframeviewbase.h"

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

KeyframeViewBase::KeyframeViewBase(QWidget *parent) :
  super(parent),
  dragging_bezier_point_(nullptr),
  currently_autoselecting_(false),
  dragging_(false),
  selection_manager_(this)
{
  SetDefaultDragMode(RubberBandDrag);
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, &KeyframeViewBase::customContextMenuRequested, this, &KeyframeViewBase::ShowContextMenu);
}

void KeyframeViewBase::DeleteSelected()
{
  MultiUndoCommand* command = new MultiUndoCommand();

  foreach (NodeKeyframe *key, GetSelectedKeyframes()) {
    command->add_child(new NodeParamRemoveKeyframeCommand(key));
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

KeyframeViewBase::NodeConnections KeyframeViewBase::AddKeyframesOfNode(Node *n)
{
  NodeConnections map;

  foreach (const QString& i, n->inputs()) {
    map.insert(i, AddKeyframesOfInput(n, i));
  }

  return map;
}

KeyframeViewBase::InputConnections KeyframeViewBase::AddKeyframesOfInput(Node* n, const QString& input)
{
  InputConnections vec;

  if (n->IsInputKeyframable(input)) {
    int arr_sz = n->InputArraySize(input);
    vec.resize(arr_sz + 1);
    for (int i=-1; i<arr_sz; i++) {
      vec[i+1] = AddKeyframesOfElement(NodeInput(n, input, i));
    }
  }

  return vec;
}

KeyframeViewBase::ElementConnections KeyframeViewBase::AddKeyframesOfElement(const NodeInput& input)
{
  const QVector<NodeKeyframeTrack>& tracks = input.node()->GetKeyframeTracks(input);
  ElementConnections vec(tracks.size());

  for (int i=0; i<tracks.size(); i++) {
    vec[i] = AddKeyframesOfTrack(NodeKeyframeTrackReference(input, i));
  }

  return vec;
}

KeyframeViewInputConnection *KeyframeViewBase::AddKeyframesOfTrack(const NodeKeyframeTrackReference& ref)
{
  KeyframeViewInputConnection *track = new KeyframeViewInputConnection(ref, this);
  connect(track, &KeyframeViewInputConnection::RequireUpdate, this, &KeyframeViewBase::Redraw);
  tracks_.append(track);
  Redraw();
  return track;
}

void KeyframeViewBase::RemoveKeyframesOfTrack(KeyframeViewInputConnection *connection)
{
  if (tracks_.removeOne(connection)) {
    delete connection;
    Redraw();
  }
}

void KeyframeViewBase::SelectAll()
{
  foreach (KeyframeViewInputConnection *track, tracks_) {
    foreach (NodeKeyframe *key, track->GetKeyframes()) {
      SelectKeyframe(key);
    }
  }
}

void KeyframeViewBase::DeselectAll()
{
  selection_manager_.ClearSelection();

  Redraw();
}

void KeyframeViewBase::Clear()
{
  if (!tracks_.isEmpty()) {
    qDeleteAll(tracks_);
    tracks_.clear();
    Redraw();
  }
}

void KeyframeViewBase::mousePressEvent(QMouseEvent *event)
{
  NodeKeyframe *key_under_cursor = selection_manager_.MousePress(event);
  if (key_under_cursor) {
    AutoSelectKeyTimeNeighbors();
  }

  BezierControlPointItem *bezier_under_cursor = dynamic_cast<BezierControlPointItem*>(itemAt(event->pos()));

  Redraw();

  if (HandPress(event) || (!bezier_under_cursor && !key_under_cursor && PlayheadPress(event))) {
    return;
  }

  if (event->button() == Qt::LeftButton) {
    if (key_under_cursor || bezier_under_cursor) {
      dragging_ = true;
      drag_start_ = mapToScene(event->pos());

      // Determine what type of item is under the cursor
      dragging_bezier_point_ = bezier_under_cursor;

      if (dragging_bezier_point_) {

        dragging_bezier_point_start_ = dragging_bezier_point_->GetCorrespondingKeyframeHandle();
        dragging_bezier_point_opposing_start_ = dragging_bezier_point_->key()->bezier_control(NodeKeyframe::get_opposing_bezier_type(dragging_bezier_point_->mode()));

      } else {

        selection_manager_.DragStart(key_under_cursor, event);

      }
    }
  }
}

void KeyframeViewBase::mouseMoveEvent(QMouseEvent *event)
{
  if (HandMove(event) || PlayheadMove(event)) {
    return;
  }

  if (event->buttons() & Qt::LeftButton) {
    if (dragging_) {
      // Calculate cursor difference and scale it
      QPointF scene_pos = mapToScene(event->pos());
      QPointF mouse_diff_scaled = GetScaledCursorPos(scene_pos - drag_start_);

      if (event->modifiers() & Qt::ShiftModifier) {
        // If holding shift, only move one axis
        mouse_diff_scaled.setY(0);
      }

      if (dragging_bezier_point_) {

        // Flip the mouse Y because bezier control points are drawn bottom to top, not top to bottom
        mouse_diff_scaled.setY(-mouse_diff_scaled.y());

        QPointF new_bezier_pos = GenerateBezierControlPosition(dragging_bezier_point_->mode(),
                                                               dragging_bezier_point_start_,
                                                               mouse_diff_scaled);

        // If the user is NOT holding control, we set the other handle to the exact negative of this handle
        QPointF new_opposing_pos;
        NodeKeyframe::BezierType opposing_type = NodeKeyframe::get_opposing_bezier_type(dragging_bezier_point_->mode());


        if (!(event->modifiers() & Qt::ControlModifier)) {
          new_opposing_pos = GenerateBezierControlPosition(opposing_type,
                                                           dragging_bezier_point_opposing_start_,
                                                           -mouse_diff_scaled);
        } else {
          new_opposing_pos = dragging_bezier_point_opposing_start_;
        }

        dragging_bezier_point_->key()->set_bezier_control(dragging_bezier_point_->mode(),
                                                          new_bezier_pos);

        dragging_bezier_point_->key()->set_bezier_control(opposing_type,
                                                          new_opposing_pos);

        // Bezier control points are parented to keyframe items making their positions relative
        // to those items. We need to map them to the scene coordinates for this to work properly.
        QPointF bezier_pos = dragging_bezier_point_->pos() + dragging_bezier_point_->parentItem()->pos();
        emit Dragged(qRound(bezier_pos.x()), qRound(bezier_pos.y()));

      } else if (selection_manager_.IsDragging()) {

        QString tip;

        /*
        // Validate movement - ensure no keyframe goes above its max point or below its min point
        FloatSlider::DisplayType display_type = FloatSlider::kNormal;

        if (IsYAxisEnabled()) {
          foreach (const KeyframeItemAndTime& keypair, dragging_keyframes_) {
            NodeKeyframe *key = keypair.key;
            Node* node = key->parent();
            const QString& input = key->input();
            double new_val = keypair.value - mouse_diff_scaled.y();
            double limited = new_val;

            if (node->HasInputProperty(input, QStringLiteral("min"))) {
              limited = qMax(limited, node->GetInputProperty(input, QStringLiteral("min")).toDouble());
            }

            if (node->HasInputProperty(input, QStringLiteral("max"))) {
              limited = qMin(limited, node->GetInputProperty(input, QStringLiteral("max")).toDouble());
            }

            if (limited != new_val) {
              mouse_diff_scaled.setY(keypair.value - limited);
            }
          }

          Node* initial_drag_input = initial_drag_item_->parent();
          const QString& initial_drag_input_id = initial_drag_item_->input();
          if (initial_drag_input->HasInputProperty(initial_drag_input_id, QStringLiteral("view"))) {
            display_type = static_cast<FloatSlider::DisplayType>(initial_drag_input->GetInputProperty(initial_drag_input_id, QStringLiteral("view")).toInt());
          }
        }

        foreach (const KeyframeItemAndTime& keypair, dragging_keyframes_) {
          if (IsYAxisEnabled()) {
            key->set_value(keypair.value - mouse_diff_scaled.y());
          }
        }



        if (IsYAxisEnabled()) {
          bool ok;
          double num_value = initial_drag_item_->value().toDouble(&ok);

          if (ok) {
            tip = QStringLiteral("%1\n");
            tip.append(FloatSlider::ValueToString(num_value, display_type, 2, true));
          }
        }
        */

        selection_manager_.DragMove(event, tip);

        Redraw();

        emit Dragged(scene_pos.x(), scene_pos.y());

      }
    }
  }
}

void KeyframeViewBase::mouseReleaseEvent(QMouseEvent *event)
{
  if (HandRelease(event) || PlayheadRelease(event)) {
    return;
  }

  if (event->button() == Qt::LeftButton) {
    if (dragging_) {
      if (dragging_bezier_point_) {
        MultiUndoCommand* command = new MultiUndoCommand();

        // Create undo command with the current bezier point and the old one
        command->add_child(new KeyframeSetBezierControlPoint(dragging_bezier_point_->key(),
                                                             dragging_bezier_point_->mode(),
                                                             dragging_bezier_point_->key()->bezier_control(dragging_bezier_point_->mode()),
                                                             dragging_bezier_point_start_));

        if (!(event->modifiers() & Qt::ControlModifier)) {
          auto opposing_type = NodeKeyframe::get_opposing_bezier_type(dragging_bezier_point_->mode());

          command->add_child(new KeyframeSetBezierControlPoint(dragging_bezier_point_->key(),
                                                               opposing_type,
                                                               dragging_bezier_point_->key()->bezier_control(opposing_type),
                                                               dragging_bezier_point_opposing_start_));
        }

        dragging_bezier_point_ = nullptr;

        Core::instance()->undo_stack()->push(command);
      } else if (selection_manager_.IsDragging()) {
        MultiUndoCommand* command = new MultiUndoCommand();

        selection_manager_.DragStop(command);
        /*if (IsYAxisEnabled()) {
          command->add_child(new NodeParamSetKeyframeValueCommand(item,
                                                                  item->value(),
                                                                  keypair.value));
        }*/

        Core::instance()->undo_stack()->push(command);
      }

      dragging_ = false;

      QToolTip::hideText();
    }
  }
}

void KeyframeViewBase::drawForeground(QPainter *painter, const QRectF &rect)
{
  int key_sz = QtUtils::QFontMetricsWidth(fontMetrics(), "Oi");
  int key_rad = key_sz/2;

  selection_manager_.ClearDrawnObjects();

  painter->setRenderHint(QPainter::Antialiasing);

  painter->setPen(Qt::black);

  foreach (KeyframeViewInputConnection *track, tracks_) {
    foreach (NodeKeyframe *key, track->GetKeyframes()) {
      QRectF key_rect(-key_rad, -key_rad, key_sz, key_sz);
      key_rect.translate(GetKeyframeSceneX(key), mapFromGlobal(QPoint(0, track->GetKeyframeY())).y());

      if (!rect.intersects(key_rect)) {
        continue;
      }

      if (IsKeyframeSelected(key)) {
        painter->setBrush(palette().highlight());
      } else {
        painter->setBrush(track->GetBrush());
      }

      selection_manager_.DeclareDrawnObject(key, key_rect);

      switch (key->type()) {
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
  }

  super::drawForeground(painter, rect);
}

void KeyframeViewBase::ScaleChangedEvent(const double &scale)
{
  super::ScaleChangedEvent(scale);

  Redraw();
}

void KeyframeViewBase::TimeTargetChangedEvent(Node *target)
{
  Redraw();
}

void KeyframeViewBase::TimebaseChangedEvent(const rational &timebase)
{
  super::TimebaseChangedEvent(timebase);

  selection_manager_.SetTimebase(timebase);
}

void KeyframeViewBase::ContextMenuEvent(Menu& m)
{
  Q_UNUSED(m)
}

void KeyframeViewBase::SelectKeyframe(NodeKeyframe *key)
{
  if (selection_manager_.Select(key)) {
    Redraw();
  }
}

void KeyframeViewBase::DeselectKeyframe(NodeKeyframe *key)
{
  if (selection_manager_.Deselect(key)) {
    Redraw();
  }
}

rational KeyframeViewBase::GetAdjustedKeyframeTime(NodeKeyframe *key)
{
  return GetAdjustedTime(key->parent(), GetTimeTarget(), key->time(), false);
}

double KeyframeViewBase::GetKeyframeSceneX(NodeKeyframe *key)
{
  return TimeToScene(GetAdjustedKeyframeTime(key));
}

rational KeyframeViewBase::CalculateNewTimeFromScreen(const rational &old_time, double cursor_diff)
{
  return rational::fromDouble(old_time.toDouble() + cursor_diff);
}

QPointF KeyframeViewBase::GenerateBezierControlPosition(const NodeKeyframe::BezierType mode, const QPointF &start_point, const QPointF &scaled_cursor_diff)
{
  QPointF new_bezier_pos = start_point;

  new_bezier_pos += scaled_cursor_diff;

  // LIMIT bezier handles from overlapping each other
  if (mode == NodeKeyframe::kInHandle) {
    if (new_bezier_pos.x() > 0) {
      new_bezier_pos.setX(0);
    }
  } else {
    if (new_bezier_pos.x() < 0) {
      new_bezier_pos.setX(0);
    }
  }

  return new_bezier_pos;
}

QPointF KeyframeViewBase::GetScaledCursorPos(const QPointF &cursor_pos)
{
  return QPointF(cursor_pos.x() / GetScale(),
                 cursor_pos.y() / GetYScale());
}

void KeyframeViewBase::ShowContextMenu()
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
    connect(properties_action, &QAction::triggered, this, &KeyframeViewBase::ShowKeyframePropertiesDialog);
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

void KeyframeViewBase::ShowKeyframePropertiesDialog()
{
  if (!GetSelectedKeyframes().isEmpty()) {
    KeyframePropertiesDialog kd(GetSelectedKeyframes(), timebase(), this);
    kd.exec();
  }
}

void KeyframeViewBase::AutoSelectKeyTimeNeighbors()
{
  if (currently_autoselecting_ || IsYAxisEnabled()) {
    return;
  }

  // Prevents infinite loop
  currently_autoselecting_ = true;

  QVector<NodeKeyframe*> copy = GetSelectedKeyframes();
  foreach (NodeKeyframe *key, copy) {
    rational key_time = key->time();

    QVector<NodeKeyframe*> keys = key->parent()->GetKeyframesAtTime(key->input(), key_time, key->element());

    foreach (NodeKeyframe* k, keys) {
      if (k != key) {
        SelectKeyframe(k);
      }
    }
  }

  currently_autoselecting_ = false;
}

void KeyframeViewBase::Redraw()
{
  viewport()->update();
}

}
