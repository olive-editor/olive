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

NodeInputDragger::NodeInputDragger() :
  input_(nullptr)
{

}

bool NodeInputDragger::IsStarted() const
{
  return input_;
}

void NodeInputDragger::Start(NodeInput *input, const rational &time, int track)
{
  Q_ASSERT(!input_);

  // Set up new drag
  input_ = input;
  time_ = time;
  track_ = track;

  // Cache current value
  start_value_ = input_->get_value_at_time_for_track(time, track);

  // Determine whether we are creating a keyframe or not
  if (input_->is_keyframing()) {
    dragging_key_ = input_->get_keyframe_at_time_on_track(time, track);
    drag_created_key_ = !dragging_key_;

    if (drag_created_key_) {
      dragging_key_ = NodeKeyframe::Create(time,
                                           start_value_,
                                           input_->get_best_keyframe_type_for_time(time, track),
                                           track);

      // We disable default signal emitting during the drag
      //input_->blockSignals(true);
      input_->insert_keyframe(dragging_key_);
      //input_->blockSignals(false);

      emit input_->KeyframeAdded(dragging_key_);
    }
  }
}

void NodeInputDragger::Drag(const QVariant& value)
{
  Q_ASSERT(input_);

  end_value_ = value;

  //input_->blockSignals(true);

  if (input_->is_keyframing()) {
    dragging_key_->set_value(value);
  } else {
    input_->set_standard_value(value, track_);
  }

  //input_->blockSignals(false);
}

void NodeInputDragger::End()
{
  if (!IsStarted()) {
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  if (input_->is_keyframing()) {
    if (drag_created_key_) {
      // We created a keyframe in this process
      new NodeParamInsertKeyframeCommand(input_, dragging_key_, true, command);
    }

    // We just set a keyframe's value
    // We do this even when inserting a keyframe because we don't actually perform an insert in this undo command
    // so this will ensure the ValueChanged() signal is sent correctly
    new NodeParamSetKeyframeValueCommand(dragging_key_, end_value_, start_value_, command);
  } else {
    // We just set the standard value
    new NodeParamSetStandardValueCommand(input_, track_, end_value_, start_value_, command);
  }

  Core::instance()->undo_stack()->push(command);

  input_ = nullptr;
}

}
