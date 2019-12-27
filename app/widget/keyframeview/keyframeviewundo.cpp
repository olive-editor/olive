#include "keyframeviewundo.h"

KeyframeSetTypeCommand::KeyframeSetTypeCommand(NodeKeyframePtr key, NodeKeyframe::Type type, QUndoCommand *parent) :
  QUndoCommand(parent),
  key_(key),
  old_type_(key->type()),
  new_type_(type)
{
}

void KeyframeSetTypeCommand::redo()
{
  key_->set_type(new_type_);
}

void KeyframeSetTypeCommand::undo()
{
  key_->set_type(old_type_);
}
