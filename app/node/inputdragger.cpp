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

void NodeInputDragger::Start(NodeInput *input, const rational &time, int track, int element)
{
  Q_ASSERT(!input_);

  // Set up new drag
  input_ = input;
  time_ = time;
  track_ = track;
  element_ = element;

  // Cache current value
  start_value_ = input_->GetValueAtTimeForTrack(time, track, element_);

  // Determine whether we are creating a keyframe or not
  if (input_->IsKeyframing(element_)) {
    dragging_key_ = input_->GetKeyframeAtTimeOnTrack(time, track, element_);
    drag_created_key_ = !dragging_key_;

    if (drag_created_key_) {
      dragging_key_ = new NodeKeyframe(time,
                                       start_value_,
                                       input_->GetBestKeyframeTypeForTime(time, track, element_),
                                       track,
                                       element_,
                                       input_);
    }
  }
}

void NodeInputDragger::Drag(QVariant value)
{
  Q_ASSERT(input_);

  if (input_->property("min").isValid()) {
    // Assumes the value is a double of some kind
    double min = input_->property("min").toDouble();
    double v = value.toDouble();
    if (v < min) {
      value = min;
    }
  }

  if (input_->property("max").isValid()) {
    double max = input_->property("max").toDouble();
    double v = value.toDouble();
    if (v > max) {
      value = max;
    }
  }

  end_value_ = value;

  //input_->blockSignals(true);

  if (input_->IsKeyframing(element_)) {
    dragging_key_->set_value(value);
  } else {
    input_->SetStandardValueOnTrack(value, track_, element_);
  }

  //input_->blockSignals(false);
}

void NodeInputDragger::End()
{
  if (!IsStarted()) {
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  if (input_->IsKeyframing(element_)) {
    if (drag_created_key_) {
      // We created a keyframe in this process
      new NodeParamInsertKeyframeCommand(input_, dragging_key_, command);
    }

    // We just set a keyframe's value
    // We do this even when inserting a keyframe because we don't actually perform an insert in this undo command
    // so this will ensure the ValueChanged() signal is sent correctly
    new NodeParamSetKeyframeValueCommand(dragging_key_, end_value_, start_value_, command);
  } else {
    // We just set the standard value
    new NodeParamSetStandardValueCommand(input_, track_, element_, end_value_, start_value_, command);
  }

  Core::instance()->undo_stack()->push(command);

  input_ = nullptr;
}

}
