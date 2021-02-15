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

#include "nodeparamviewundo.h"

#include "node/node.h"
#include "project/item/sequence/sequence.h"

namespace olive {

NodeParamSetKeyframingCommand::NodeParamSetKeyframingCommand(const NodeInput &input, bool setting) :
  input_(input),
  setting_(setting)
{
  Q_ASSERT(setting != input_.IsKeyframing());
}

Project *NodeParamSetKeyframingCommand::GetRelevantProject() const
{
  return input_.node()->project();
}

void NodeParamSetKeyframingCommand::redo()
{
  input_.node()->SetInputIsKeyframing(input_, setting_);
}

void NodeParamSetKeyframingCommand::undo()
{
  input_.node()->SetInputIsKeyframing(input_, !setting_);
}

NodeParamSetKeyframeValueCommand::NodeParamSetKeyframeValueCommand(NodeKeyframe* key, const QVariant& value) :
  key_(key),
  old_value_(key_->value()),
  new_value_(value)
{
}

NodeParamSetKeyframeValueCommand::NodeParamSetKeyframeValueCommand(NodeKeyframe* key, const QVariant &new_value, const QVariant &old_value) :
  key_(key),
  old_value_(old_value),
  new_value_(new_value)
{

}

Project *NodeParamSetKeyframeValueCommand::GetRelevantProject() const
{
  return key_->parent()->project();
}

void NodeParamSetKeyframeValueCommand::redo()
{
  key_->set_value(new_value_);
}

void NodeParamSetKeyframeValueCommand::undo()
{
  key_->set_value(old_value_);
}

NodeParamInsertKeyframeCommand::NodeParamInsertKeyframeCommand(Node* node, NodeKeyframe* keyframe) :
  input_(node),
  keyframe_(keyframe)
{
  // Take ownership of the keyframe
  undo();
}

Project *NodeParamInsertKeyframeCommand::GetRelevantProject() const
{
  return input_->project();
}

void NodeParamInsertKeyframeCommand::redo()
{
  keyframe_->setParent(input_);
}

void NodeParamInsertKeyframeCommand::undo()
{
  keyframe_->setParent(&memory_manager_);
}

NodeParamRemoveKeyframeCommand::NodeParamRemoveKeyframeCommand(NodeKeyframe* keyframe) :
  input_(keyframe->parent()),
  keyframe_(keyframe)
{
}

Project *NodeParamRemoveKeyframeCommand::GetRelevantProject() const
{
  return input_->project();
}

void NodeParamRemoveKeyframeCommand::redo()
{
  // Removes from input
  keyframe_->setParent(&memory_manager_);
}

void NodeParamRemoveKeyframeCommand::undo()
{
  keyframe_->setParent(input_);
}

NodeParamSetKeyframeTimeCommand::NodeParamSetKeyframeTimeCommand(NodeKeyframe* key, const rational &time) :
  key_(key),
  old_time_(key->time()),
  new_time_(time)
{
}

NodeParamSetKeyframeTimeCommand::NodeParamSetKeyframeTimeCommand(NodeKeyframe* key, const rational &new_time, const rational &old_time) :
  key_(key),
  old_time_(old_time),
  new_time_(new_time)
{
}

Project *NodeParamSetKeyframeTimeCommand::GetRelevantProject() const
{
  return key_->parent()->project();
}

void NodeParamSetKeyframeTimeCommand::redo()
{
  key_->set_time(new_time_);
}

void NodeParamSetKeyframeTimeCommand::undo()
{
  key_->set_time(old_time_);
}

NodeParamSetStandardValueCommand::NodeParamSetStandardValueCommand(const NodeKeyframeTrackReference& input, const QVariant &value) :
  ref_(input),
  old_value_(ref_.input().node()->GetStandardValue(ref_.input())),
  new_value_(value)
{
}

NodeParamSetStandardValueCommand::NodeParamSetStandardValueCommand(const NodeKeyframeTrackReference& input, const QVariant &new_value, const QVariant &old_value) :
  ref_(input),
  old_value_(old_value),
  new_value_(new_value)
{
}

Project *NodeParamSetStandardValueCommand::GetRelevantProject() const
{
  return ref_.input().node()->project();
}

void NodeParamSetStandardValueCommand::redo()
{
  ref_.input().node()->SetSplitStandardValueOnTrack(ref_, new_value_);
}

void NodeParamSetStandardValueCommand::undo()
{
  ref_.input().node()->SetSplitStandardValueOnTrack(ref_, old_value_);
}

Project *NodeParamArrayInsertCommand::GetRelevantProject() const
{
  return input_.node()->project();
}

}
