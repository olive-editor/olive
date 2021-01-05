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

NodeParamSetKeyframingCommand::NodeParamSetKeyframingCommand(NodeInput *input, int element, bool setting, QUndoCommand *parent) :
  UndoCommand(parent),
  input_(input),
  setting_(setting),
  element_(element)
{
  Q_ASSERT(setting != input_->IsKeyframing());
}

Project *NodeParamSetKeyframingCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(input_->parent()->parent())->project();
}

void NodeParamSetKeyframingCommand::redo_internal()
{
  input_->SetIsKeyframing(setting_, element_);
}

void NodeParamSetKeyframingCommand::undo_internal()
{
  input_->SetIsKeyframing(!setting_, element_);
}

NodeParamSetKeyframeValueCommand::NodeParamSetKeyframeValueCommand(NodeKeyframe* key, const QVariant& value, QUndoCommand* parent) :
  UndoCommand(parent),
  key_(key),
  old_value_(key_->value()),
  new_value_(value)
{
}

NodeParamSetKeyframeValueCommand::NodeParamSetKeyframeValueCommand(NodeKeyframe* key, const QVariant &new_value, const QVariant &old_value, QUndoCommand *parent) :
  UndoCommand(parent),
  key_(key),
  old_value_(old_value),
  new_value_(new_value)
{

}

Project *NodeParamSetKeyframeValueCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(key_->parent()->parent()->parent()->parent())->project();
}

void NodeParamSetKeyframeValueCommand::redo_internal()
{
  key_->set_value(new_value_);
}

void NodeParamSetKeyframeValueCommand::undo_internal()
{
  key_->set_value(old_value_);
}

NodeParamInsertKeyframeCommand::NodeParamInsertKeyframeCommand(NodeInput *input, NodeKeyframe* keyframe, QUndoCommand* parent) :
  UndoCommand(parent),
  input_(input),
  keyframe_(keyframe)
{
  keyframe_->setParent(&memory_manager_);
}

Project *NodeParamInsertKeyframeCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(input_->parent()->parent())->project();
}

void NodeParamInsertKeyframeCommand::redo_internal()
{
  keyframe_->setParent(input_);
}

void NodeParamInsertKeyframeCommand::undo_internal()
{
  keyframe_->setParent(&memory_manager_);
}

NodeParamRemoveKeyframeCommand::NodeParamRemoveKeyframeCommand(NodeKeyframe* keyframe, QUndoCommand *parent) :
  UndoCommand(parent),
  input_(keyframe->parent()),
  keyframe_(keyframe)
{
}

Project *NodeParamRemoveKeyframeCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(input_->parent()->parent())->project();
}

void NodeParamRemoveKeyframeCommand::redo_internal()
{
  // Removes from input
  keyframe_->setParent(&memory_manager_);
}

void NodeParamRemoveKeyframeCommand::undo_internal()
{
  keyframe_->setParent(input_);
}

NodeParamSetKeyframeTimeCommand::NodeParamSetKeyframeTimeCommand(NodeKeyframe* key, const rational &time, QUndoCommand *parent) :
  UndoCommand(parent),
  key_(key),
  old_time_(key->time()),
  new_time_(time)
{
}

NodeParamSetKeyframeTimeCommand::NodeParamSetKeyframeTimeCommand(NodeKeyframe* key, const rational &new_time, const rational &old_time, QUndoCommand *parent) :
  UndoCommand(parent),
  key_(key),
  old_time_(old_time),
  new_time_(new_time)
{
}

Project *NodeParamSetKeyframeTimeCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(key_->parent()->parent()->parent())->project();
}

void NodeParamSetKeyframeTimeCommand::redo_internal()
{
  key_->set_time(new_time_);
}

void NodeParamSetKeyframeTimeCommand::undo_internal()
{
  key_->set_time(old_time_);
}

NodeParamSetStandardValueCommand::NodeParamSetStandardValueCommand(NodeInput *input, int track, int element, const QVariant &value, QUndoCommand *parent) :
  UndoCommand(parent),
  input_(input),
  element_(element),
  track_(track),
  old_value_(input_->GetStandardValue(element_)),
  new_value_(value)
{
}

NodeParamSetStandardValueCommand::NodeParamSetStandardValueCommand(NodeInput *input, int track, int element, const QVariant &new_value, const QVariant &old_value, QUndoCommand *parent) :
  UndoCommand(parent),
  input_(input),
  element_(element),
  track_(track),
  old_value_(old_value),
  new_value_(new_value)
{
}

Project *NodeParamSetStandardValueCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(input_->parent()->parent())->project();
}

void NodeParamSetStandardValueCommand::redo_internal()
{
  input_->SetStandardValueOnTrack(new_value_, track_, element_);
}

void NodeParamSetStandardValueCommand::undo_internal()
{
  input_->SetStandardValueOnTrack(old_value_, track_, element_);
}

}
