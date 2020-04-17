/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

OLIVE_NAMESPACE_ENTER

NodeParamSetKeyframingCommand::NodeParamSetKeyframingCommand(NodeInput *input, bool setting, QUndoCommand *parent) :
  UndoCommand(parent),
  input_(input),
  setting_(setting)
{
  Q_ASSERT(setting != input_->is_keyframing());
}

Project *NodeParamSetKeyframingCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(input_->parentNode()->parent())->project();
}

void NodeParamSetKeyframingCommand::redo_internal()
{
  input_->set_is_keyframing(setting_);
}

void NodeParamSetKeyframingCommand::undo_internal()
{
  input_->set_is_keyframing(!setting_);
}

NodeParamSetKeyframeValueCommand::NodeParamSetKeyframeValueCommand(NodeKeyframePtr key, const QVariant& value, QUndoCommand* parent) :
  UndoCommand(parent),
  key_(key),
  old_value_(key_->value()),
  new_value_(value)
{
}

NodeParamSetKeyframeValueCommand::NodeParamSetKeyframeValueCommand(NodeKeyframePtr key, const QVariant &new_value, const QVariant &old_value, QUndoCommand *parent) :
  UndoCommand(parent),
  key_(key),
  old_value_(old_value),
  new_value_(new_value)
{

}

Project *NodeParamSetKeyframeValueCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(key_->parent()->parentNode()->parent())->project();
}

void NodeParamSetKeyframeValueCommand::redo_internal()
{
  key_->set_value(new_value_);
}

void NodeParamSetKeyframeValueCommand::undo_internal()
{
  key_->set_value(old_value_);
}

NodeParamInsertKeyframeCommand::NodeParamInsertKeyframeCommand(NodeInput *input, NodeKeyframePtr keyframe, QUndoCommand* parent) :
  UndoCommand(parent),
  input_(input),
  keyframe_(keyframe),
  done_(false)
{
}

NodeParamInsertKeyframeCommand::NodeParamInsertKeyframeCommand(NodeInput *input, NodeKeyframePtr keyframe, bool already_done, QUndoCommand *parent) :
  UndoCommand(parent),
  input_(input),
  keyframe_(keyframe),
  done_(already_done)
{
}

Project *NodeParamInsertKeyframeCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(input_->parentNode()->parent())->project();
}

void NodeParamInsertKeyframeCommand::redo_internal()
{
  if (!done_) {
    input_->insert_keyframe(keyframe_);
  }
}

void NodeParamInsertKeyframeCommand::undo_internal()
{
  input_->remove_keyframe(keyframe_);
  done_ = false;
}

NodeParamRemoveKeyframeCommand::NodeParamRemoveKeyframeCommand(NodeInput *input, NodeKeyframePtr keyframe, QUndoCommand *parent) :
  UndoCommand(parent),
  input_(input),
  keyframe_(keyframe)
{
}

Project *NodeParamRemoveKeyframeCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(input_->parentNode()->parent())->project();
}

void NodeParamRemoveKeyframeCommand::redo_internal()
{
  input_->remove_keyframe(keyframe_);
}

void NodeParamRemoveKeyframeCommand::undo_internal()
{
  input_->insert_keyframe(keyframe_);
}

NodeParamSetKeyframeTimeCommand::NodeParamSetKeyframeTimeCommand(NodeKeyframePtr key, const rational &time, QUndoCommand *parent) :
  UndoCommand(parent),
  key_(key),
  old_time_(key->time()),
  new_time_(time)
{
}

NodeParamSetKeyframeTimeCommand::NodeParamSetKeyframeTimeCommand(NodeKeyframePtr key, const rational &new_time, const rational &old_time, QUndoCommand *parent) :
  UndoCommand(parent),
  key_(key),
  old_time_(old_time),
  new_time_(new_time)
{
}

Project *NodeParamSetKeyframeTimeCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(key_->parent()->parentNode()->parent())->project();
}

void NodeParamSetKeyframeTimeCommand::redo_internal()
{
  key_->set_time(new_time_);
}

void NodeParamSetKeyframeTimeCommand::undo_internal()
{
  key_->set_time(old_time_);
}

NodeParamSetStandardValueCommand::NodeParamSetStandardValueCommand(NodeInput *input, int track, const QVariant &value, QUndoCommand *parent) :
  UndoCommand(parent),
  input_(input),
  track_(track),
  old_value_(input_->get_standard_value()),
  new_value_(value)
{
}

NodeParamSetStandardValueCommand::NodeParamSetStandardValueCommand(NodeInput *input, int track, const QVariant &new_value, const QVariant &old_value, QUndoCommand *parent) :
  UndoCommand(parent),
  input_(input),
  track_(track),
  old_value_(old_value),
  new_value_(new_value)
{
}

Project *NodeParamSetStandardValueCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(input_->parentNode()->parent())->project();
}

void NodeParamSetStandardValueCommand::redo_internal()
{
  input_->set_standard_value(new_value_, track_);
}

void NodeParamSetStandardValueCommand::undo_internal()
{
  input_->set_standard_value(old_value_, track_);
}

OLIVE_NAMESPACE_EXIT
