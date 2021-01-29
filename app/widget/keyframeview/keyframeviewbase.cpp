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

#include "keyframeviewbase.h"

#include <QMouseEvent>
#include <QToolTip>
#include <QVBoxLayout>

#include "dialog/keyframeproperties/keyframeproperties.h"
#include "keyframeviewundo.h"
#include "node/node.h"
#include "widget/menu/menu.h"
#include "widget/menu/menushared.h"
#include "widget/nodeparamview/nodeparamviewundo.h"

namespace olive {

KeyframeViewBase::KeyframeViewBase(QWidget *parent) :
  TimeBasedView(parent),
  dragging_bezier_point_(nullptr),
  currently_autoselecting_(false),
  dragging_(false)
{
  SetDefaultDragMode(RubberBandDrag);
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, &KeyframeViewBase::customContextMenuRequested, this, &KeyframeViewBase::ShowContextMenu);
  connect(scene(), &QGraphicsScene::selectionChanged, this, &KeyframeViewBase::AutoSelectKeyTimeNeighbors);
}

void KeyframeViewBase::Clear()
{
  QMap<NodeKeyframe*, KeyframeViewItem*>::iterator iterator;

  for (iterator=item_map_.begin();iterator!=item_map_.end();iterator++) {
    delete iterator.value();
  }

  item_map_.clear();
}

void KeyframeViewBase::DeleteSelected()
{
  MultiUndoCommand* command = new MultiUndoCommand();

  QMap<NodeKeyframe*, KeyframeViewItem*>::const_iterator i;

  for (i=item_map_.constBegin(); i!=item_map_.constEnd(); i++) {
    if (i.value()->isSelected()) {
      command->add_child(new NodeParamRemoveKeyframeCommand(i.key()));
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void KeyframeViewBase::AddKeyframesOfNode(Node *n)
{
  foreach (NodeInput* i, n->inputs()) {
    AddKeyframesOfInput(i);
  }
}

void KeyframeViewBase::AddKeyframesOfInput(NodeInput *input)
{
  if (!input->IsKeyframable()) {
    return;
  }

  for (int i=-1; i<input->ArraySize(); i++) {
    AddKeyframesOfElement(input, i);
  }
}

void KeyframeViewBase::AddKeyframesOfElement(NodeInput *input, int element)
{
  const QVector<NodeKeyframeTrack>& tracks = input->GetKeyframeTracks(element);

  for (int i=0; i<tracks.size(); i++) {
    AddKeyframesOfTrack(input, element, i);
  }
}

void KeyframeViewBase::AddKeyframesOfTrack(NodeInput *input, int element, int track)
{
  const QVector<NodeKeyframeTrack>& tracks = input->GetKeyframeTracks(element);
  const NodeKeyframeTrack& t = tracks.at(track);

  foreach (NodeKeyframe* key, t) {
    AddKeyframe(key);
  }
}

void KeyframeViewBase::RemoveKeyframesOfNode(Node *n)
{
  foreach (NodeInput* i, n->inputs()) {
    RemoveKeyframesOfInput(i);
  }
}

void KeyframeViewBase::RemoveKeyframesOfInput(NodeInput *input)
{
  if (!input->IsKeyframable()) {
    return;
  }

  for (int i=-1; i<input->ArraySize(); i++) {
    RemoveKeyframesOfElement(input, i);
  }
}

void KeyframeViewBase::RemoveKeyframesOfElement(NodeInput *input, int element)
{
  const QVector<NodeKeyframeTrack>& tracks = input->GetKeyframeTracks(element);

  for (int i=0; i<tracks.size(); i++) {
    RemoveKeyframesOfTrack(input, element, i);
  }
}

void KeyframeViewBase::RemoveKeyframesOfTrack(NodeInput *input, int element, int track)
{
  const QVector<NodeKeyframeTrack>& tracks = input->GetKeyframeTracks(element);
  const NodeKeyframeTrack& t = tracks.at(track);

  foreach (NodeKeyframe* key, t) {
    RemoveKeyframe(key);
  }
}

void KeyframeViewBase::SelectAll()
{
  for (auto it=item_map_.cbegin(); it!=item_map_.cend(); it++) {
    it.value()->setSelected(true);
  }
}

void KeyframeViewBase::DeselectAll()
{
  for (auto it=item_map_.cbegin(); it!=item_map_.cend(); it++) {
    it.value()->setSelected(false);
  }
}

void KeyframeViewBase::RemoveKeyframe(NodeKeyframe* key)
{
  KeyframeAboutToBeRemoved(key);

  delete item_map_.take(key);
}

KeyframeViewItem *KeyframeViewBase::AddKeyframe(NodeKeyframe* key)
{
  KeyframeViewItem* item = item_map_.value(key);

  if (!item) {
    item = new KeyframeViewItem(key);
    item->SetTimeTarget(GetTimeTarget());
    item->SetScale(GetScale());
    item_map_.insert(key, item);
    scene()->addItem(item);
  }

  return item;
}

void KeyframeViewBase::mousePressEvent(QMouseEvent *event)
{
  QGraphicsItem* item_under_cursor = itemAt(event->pos());

  if (HandPress(event) || (!item_under_cursor && PlayheadPress(event))) {
    return;
  }

  active_tool_ = Core::instance()->tool();

  if (event->button() == Qt::LeftButton) {
    QGraphicsView::mousePressEvent(event);

    if (active_tool_ == Tool::kPointer) {
      if (item_under_cursor) {

        dragging_ = true;
        drag_start_ = mapToScene(event->pos());

        // Determine what type of item is under the cursor
        dragging_bezier_point_ = dynamic_cast<BezierControlPointItem*>(item_under_cursor);

        if (dragging_bezier_point_) {

          dragging_bezier_point_start_ = dragging_bezier_point_->GetCorrespondingKeyframeHandle();
          dragging_bezier_point_opposing_start_ = dragging_bezier_point_->key()->bezier_control(NodeKeyframe::get_opposing_bezier_type(dragging_bezier_point_->mode()));

        } else {

          QList<QGraphicsItem*> selected_items = scene()->selectedItems();

          selected_keys_.resize(selected_items.size());

          initial_drag_item_ = static_cast<KeyframeViewItem*>(item_under_cursor);

          for (int i=0;i<selected_items.size();i++) {
            KeyframeViewItem* key = static_cast<KeyframeViewItem*>(selected_items.at(i));

            selected_keys_.replace(i, {key,
                                       key->x(),
                                       GetAdjustedTime(key->key()->parent()->parent(), GetTimeTarget(), key->key()->time(), false),
                                       key->key()->value().toDouble()});
          }
        }
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
    QGraphicsView::mouseMoveEvent(event);

    if (dragging_) {
      // Calculate cursor difference and scale it
      QPointF mouse_diff_scaled = GetScaledCursorPos(mapToScene(event->pos()) - drag_start_);

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

        emit Dragged(qRound(dragging_bezier_point_->x()), qRound(dragging_bezier_point_->y()));

      } else if (!selected_keys_.isEmpty()) {

        // Validate movement - ensure no keyframe goes above its max point or below its min point
        FloatSlider::DisplayType display_type = FloatSlider::kNormal;

        if (IsYAxisEnabled()) {
          foreach (const KeyframeItemAndTime& keypair, selected_keys_) {
            NodeInput* input = keypair.key->key()->parent();
            QList<QByteArray> properties = input->dynamicPropertyNames();

            double new_val = keypair.value - mouse_diff_scaled.y();
            double limited = new_val;

            if (properties.contains("min")) {
              limited = qMax(limited, input->property("min").toDouble());
            }

            if (properties.contains("max")) {
              limited = qMin(limited, input->property("max").toDouble());
            }

            if (limited != new_val) {
              mouse_diff_scaled.setY(keypair.value - limited);
            }
          }

          NodeInput* initial_drag_input = initial_drag_item_->key()->parent();
          if (initial_drag_input->dynamicPropertyNames().contains("view")) {
            display_type = static_cast<FloatSlider::DisplayType>(initial_drag_input->property("view").toInt());
          }
        }

        foreach (const KeyframeItemAndTime& keypair, selected_keys_) {
          rational node_time = GetAdjustedTime(GetTimeTarget(),
                                               keypair.key->key()->parent()->parent(),
                                               CalculateNewTimeFromScreen(keypair.time, mouse_diff_scaled.x()),
                                               true);

          keypair.key->key()->set_time(node_time);

          if (IsYAxisEnabled()) {
            keypair.key->key()->set_value(keypair.value - mouse_diff_scaled.y());
          }
        }

        // Show information about this keyframe
        QString tip = Timecode::time_to_timecode(initial_drag_item_->key()->time(), timebase(),
                                                 Core::instance()->GetTimecodeDisplay(), false);

        if (IsYAxisEnabled()) {
          bool ok;
          double num_value = initial_drag_item_->key()->value().toDouble(&ok);

          if (ok) {
            tip.append('\n');
            tip.append(FloatSlider::ValueToString(num_value, display_type, 2, true));
          }

          // Force viewport to update since Qt might try to optimize it out if the keyframe is
          // offscreen
          viewport()->update();
        }

        QToolTip::hideText();
        QToolTip::showText(QCursor::pos(), tip);

        emit Dragged(qRound(initial_drag_item_->x()), qRound(initial_drag_item_->y()));

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
    QGraphicsView::mouseReleaseEvent(event);

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
      } else if (!selected_keys_.isEmpty()) {
        MultiUndoCommand* command = new MultiUndoCommand();

        foreach (const KeyframeItemAndTime& keypair, selected_keys_) {
          NodeKeyframe* item = keypair.key->key();

          // Commit movement
          command->add_child(new NodeParamSetKeyframeTimeCommand(item,
                                                                 item->time(),
                                                                 keypair.time));

          // Commit value if we're setting a value
          if (IsYAxisEnabled()) {
            command->add_child(new NodeParamSetKeyframeValueCommand(item,
                                                                    item->value(),
                                                                    keypair.value));
          }
        }

        Core::instance()->undo_stack()->push(command);
      }

      selected_keys_.clear();

      dragging_ = false;

      QToolTip::hideText();
    }
  }
}

void KeyframeViewBase::ScaleChangedEvent(const double &scale)
{
  TimeBasedView::ScaleChangedEvent(scale);

  for (auto iterator=item_map_.begin();iterator!=item_map_.end();iterator++) {
    iterator.value()->SetScale(scale);
  }
}

const QMap<NodeKeyframe *, KeyframeViewItem *> &KeyframeViewBase::item_map() const
{
  return item_map_;
}

void KeyframeViewBase::KeyframeAboutToBeRemoved(NodeKeyframe *)
{
}

void KeyframeViewBase::TimeTargetChangedEvent(Node *target)
{
  QMap<NodeKeyframe*, KeyframeViewItem*>::const_iterator i;

  for (i=item_map_.begin();i!=item_map_.end();i++) {
    i.value()->SetTimeTarget(target);
  }
}

void KeyframeViewBase::ContextMenuEvent(Menu& m)
{
  Q_UNUSED(m)
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

  QList<QGraphicsItem*> items = scene()->selectedItems();
  if (!items.isEmpty()) {
    bool all_keys_are_same_type = true;
    NodeKeyframe::Type type = static_cast<KeyframeViewItem*>(items.first())->key()->type();

    for (int i=1;i<items.size();i++) {
      KeyframeViewItem* key_item = static_cast<KeyframeViewItem*>(items.at(i));
      KeyframeViewItem* prev_item = static_cast<KeyframeViewItem*>(items.at(i-1));

      if (key_item->key()->type() != prev_item->key()->type()) {
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

  ContextMenuEvent(m);

  if (!items.isEmpty()) {
    m.addSeparator();

    QAction* properties_action = m.addAction(tr("P&roperties"));
    connect(properties_action, &QAction::triggered, this, &KeyframeViewBase::ShowKeyframePropertiesDialog);
  }

  QAction* selected = m.exec(QCursor::pos());

  // Process keyframe type changes
  if (!items.isEmpty()) {
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
      foreach (QGraphicsItem* item, items) {
        command->add_child(new KeyframeSetTypeCommand(static_cast<KeyframeViewItem*>(item)->key(),
                                                      new_type));
      }
      Core::instance()->undo_stack()->pushIfHasChildren(command);
    }
  }
}

void KeyframeViewBase::ShowKeyframePropertiesDialog()
{
  QList<QGraphicsItem*> items = scene()->selectedItems();
  QVector<NodeKeyframe*> keys;

  foreach (QGraphicsItem* item, items) {
    keys.append(static_cast<KeyframeViewItem*>(item)->key());
  }

  if (!keys.isEmpty()) {
    KeyframePropertiesDialog kd(keys, timebase(), this);
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

  QList<QGraphicsItem*> selected_items = scene()->selectedItems();

  foreach (QGraphicsItem* g, selected_items) {
    KeyframeViewItem* key_item = static_cast<KeyframeViewItem*>(g);

    rational key_time = key_item->key()->time();

    QVector<NodeKeyframe*> keys = key_item->key()->parent()->GetKeyframesAtTime(key_time, key_item->key()->element());

    foreach (NodeKeyframe* k, keys) {
      if (k == key_item->key()) {
        continue;
      }

      // Ensure this key is not already selected
      KeyframeViewItem* item = item_map_.value(k);

      item->setSelected(true);
    }
  }

  currently_autoselecting_ = false;
}

}
