#include "nodeparamviewundo.h"

NodeParamSetKeyframing::NodeParamSetKeyframing(NodeInput *input, bool setting, QUndoCommand *parent) :
  QUndoCommand(parent),
  input_(input),
  setting_(setting)
{
  Q_ASSERT(setting != input_->is_keyframing());
}

void NodeParamSetKeyframing::redo()
{
  input_->set_is_keyframing(setting_);
}

void NodeParamSetKeyframing::undo()
{
  input_->set_is_keyframing(!setting_);
}
