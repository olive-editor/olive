#include "nodeparamviewundo.h"

NodeParamSetKeyframingCommand::NodeParamSetKeyframingCommand(NodeInput *input, bool setting, QUndoCommand *parent) :
  QUndoCommand(parent),
  input_(input),
  setting_(setting)
{
  Q_ASSERT(setting != input_->is_keyframing());
}

void NodeParamSetKeyframingCommand::redo()
{
  input_->set_is_keyframing(setting_);
}

void NodeParamSetKeyframingCommand::undo()
{
  input_->set_is_keyframing(!setting_);
}

NodeParamSetKeyframeValueCommand::NodeParamSetKeyframeValueCommand(NodeKeyframePtr key, const QVariant& value, QUndoCommand* parent) :
  QUndoCommand(parent),
  key_(key),
  old_value_(key_->value()),
  new_value_(value)
{
}

void NodeParamSetKeyframeValueCommand::redo()
{
  key_->set_value(new_value_);
}

void NodeParamSetKeyframeValueCommand::undo()
{
  key_->set_value(old_value_);
}

NodeParamInsertKeyframeCommand::NodeParamInsertKeyframeCommand(NodeInput *input, NodeKeyframePtr keyframe, QUndoCommand* parent) :
  QUndoCommand(parent),
  input_(input),
  keyframe_(keyframe)
{
}

void NodeParamInsertKeyframeCommand::redo()
{
  input_->insert_keyframe(keyframe_);
}

void NodeParamInsertKeyframeCommand::undo()
{
  input_->remove_keyframe(keyframe_);
}

NodeParamRemoveKeyframeCommand::NodeParamRemoveKeyframeCommand(NodeInput *input, NodeKeyframePtr keyframe, QUndoCommand *parent) :
  QUndoCommand(parent),
  input_(input),
  keyframe_(keyframe)
{
}

void NodeParamRemoveKeyframeCommand::redo()
{
  input_->remove_keyframe(keyframe_);
}

void NodeParamRemoveKeyframeCommand::undo()
{
  input_->insert_keyframe(keyframe_);
}

NodeParamSetKeyframeTimeCommand::NodeParamSetKeyframeTimeCommand(NodeKeyframePtr key, const rational &time, QUndoCommand *parent) :
  QUndoCommand(parent),
  key_(key),
  old_time_(key->time()),
  new_time_(time)
{
}

void NodeParamSetKeyframeTimeCommand::redo()
{
  key_->set_time(new_time_);
}

void NodeParamSetKeyframeTimeCommand::undo()
{
  key_->set_time(old_time_);
}
