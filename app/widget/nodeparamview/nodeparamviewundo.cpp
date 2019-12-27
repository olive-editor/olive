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

NodeParamSetKeyframeValueCommand::NodeParamSetKeyframeValueCommand(NodeKeyframePtr key, const QVariant &new_value, const QVariant &old_value, QUndoCommand *parent) :
  QUndoCommand(parent),
  key_(key),
  old_value_(old_value),
  new_value_(new_value)
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
  keyframe_(keyframe),
  done_(false)
{
}

NodeParamInsertKeyframeCommand::NodeParamInsertKeyframeCommand(NodeInput *input, NodeKeyframePtr keyframe, bool already_done, QUndoCommand *parent) :
  QUndoCommand(parent),
  input_(input),
  keyframe_(keyframe),
  done_(already_done)
{
}

void NodeParamInsertKeyframeCommand::redo()
{
  if (!done_) {
    input_->insert_keyframe(keyframe_);
  }
}

void NodeParamInsertKeyframeCommand::undo()
{
  input_->remove_keyframe(keyframe_);
  done_ = false;
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

NodeParamSetKeyframeTimeCommand::NodeParamSetKeyframeTimeCommand(NodeKeyframePtr key, const rational &new_time, const rational &old_time, QUndoCommand *parent) :
  QUndoCommand(parent),
  key_(key),
  old_time_(old_time),
  new_time_(new_time)
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

NodeParamSetStandardValueCommand::NodeParamSetStandardValueCommand(NodeInput *input, const QVariant &value, QUndoCommand *parent) :
  QUndoCommand(parent),
  input_(input),
  old_value_(input_->get_standard_value()),
  new_value_(value)
{
}

NodeParamSetStandardValueCommand::NodeParamSetStandardValueCommand(NodeInput *input, const QVariant &new_value, const QVariant &old_value, QUndoCommand *parent) :
  QUndoCommand(parent),
  input_(input),
  old_value_(old_value),
  new_value_(new_value)
{
}

void NodeParamSetStandardValueCommand::redo()
{
  input_->set_standard_value(new_value_);
}

void NodeParamSetStandardValueCommand::undo()
{
  input_->set_standard_value(old_value_);
}
