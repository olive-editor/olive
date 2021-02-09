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

#include "inputdragger.h"

#include "core.h"
#include "node.h"
#include "widget/nodeparamview/nodeparamviewundo.h"

namespace olive {

NodeInputDragger::NodeInputDragger()
{

}

bool NodeInputDragger::IsStarted() const
{
  return input_.IsValid();
}

void NodeInputDragger::Start(const NodeKeyframeTrackReference &input, const rational &time)
{
  Q_ASSERT(!IsStarted());

  // Set up new drag
  input_ = input;
  time_ = time;

  Node* node = input_.input().node();

  // Cache current value
  start_value_ = node->GetSplitValueAtTimeOnTrack(input_, time);

  // Determine whether we are creating a keyframe or not
  if (input_.input().IsKeyframing()) {
    dragging_key_ = node->GetKeyframeAtTimeOnTrack(input_, time);
    drag_created_key_ = !dragging_key_;

    if (drag_created_key_) {
      dragging_key_ = new NodeKeyframe(time,
                                       start_value_,
                                       node->GetBestKeyframeTypeForTimeOnTrack(input_, time),
                                       input_.track(),
                                       input_.input().element(),
                                       input_.input().input(),
                                       node);
    }
  }
}

void NodeInputDragger::Drag(QVariant value)
{
  Q_ASSERT(IsStarted());

  Node* node = input_.input().node();
  const QString& input = input_.input().input();

  if (node->HasInputProperty(input, QStringLiteral("min"))) {
    // Assumes the value is a double of some kind
    double min = node->GetInputProperty(input, QStringLiteral("min")).toDouble();
    double v = value.toDouble();
    if (v < min) {
      value = min;
    }
  }

  if (node->HasInputProperty(input, QStringLiteral("max"))) {
    double max = node->GetInputProperty(input, QStringLiteral("max")).toDouble();
    double v = value.toDouble();
    if (v > max) {
      value = max;
    }
  }

  end_value_ = value;

  //input_->blockSignals(true);

  if (input_.input().IsKeyframing()) {
    dragging_key_->set_value(value);
  } else {
    node->SetSplitStandardValueOnTrack(input_, value);
  }

  //input_->blockSignals(false);
}

void NodeInputDragger::End()
{
  if (!IsStarted()) {
    return;
  }

  MultiUndoCommand* command = new MultiUndoCommand();

  if (input_.input().node()->IsInputKeyframing(input_.input())) {
    if (drag_created_key_) {
      // We created a keyframe in this process
      command->add_child(new NodeParamInsertKeyframeCommand(input_.input().node(), dragging_key_));
    }

    // We just set a keyframe's value
    // We do this even when inserting a keyframe because we don't actually perform an insert in this undo command
    // so this will ensure the ValueChanged() signal is sent correctly
    command->add_child(new NodeParamSetKeyframeValueCommand(dragging_key_, end_value_, start_value_));
  } else {
    // We just set the standard value
    command->add_child(new NodeParamSetStandardValueCommand(input_, end_value_, start_value_));
  }

  Core::instance()->undo_stack()->push(command);

  input_.Reset();
}

}
