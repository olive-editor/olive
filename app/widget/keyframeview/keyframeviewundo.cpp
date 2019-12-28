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

KeyframeSetBezierControlPoint::KeyframeSetBezierControlPoint(NodeKeyframePtr key, NodeKeyframe::BezierType mode, const QPointF& point, QUndoCommand *parent) :
  QUndoCommand(parent),
  key_(key),
  mode_(mode),
  old_point_(key->bezier_control(mode_)),
  new_point_(point)
{
}

KeyframeSetBezierControlPoint::KeyframeSetBezierControlPoint(NodeKeyframePtr key, NodeKeyframe::BezierType mode, const QPointF &new_point, const QPointF &old_point, QUndoCommand *parent) :
  QUndoCommand(parent),
  key_(key),
  mode_(mode),
  old_point_(old_point),
  new_point_(new_point)
{
}

void KeyframeSetBezierControlPoint::redo()
{
  key_->set_bezier_control(mode_, new_point_);
}

void KeyframeSetBezierControlPoint::undo()
{
  key_->set_bezier_control(mode_, old_point_);
}
