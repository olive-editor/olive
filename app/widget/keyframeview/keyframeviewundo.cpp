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

#include "keyframeviewundo.h"

OLIVE_NAMESPACE_ENTER

KeyframeSetTypeCommand::KeyframeSetTypeCommand(NodeKeyframePtr key, NodeKeyframe::Type type, QUndoCommand *parent) :
  UndoCommand(parent),
  key_(key),
  old_type_(key->type()),
  new_type_(type)
{
}

void KeyframeSetTypeCommand::redo_internal()
{
  key_->set_type(new_type_);
}

void KeyframeSetTypeCommand::undo_internal()
{
  key_->set_type(old_type_);
}

KeyframeSetBezierControlPoint::KeyframeSetBezierControlPoint(NodeKeyframePtr key, NodeKeyframe::BezierType mode, const QPointF& point, QUndoCommand *parent) :
  UndoCommand(parent),
  key_(key),
  mode_(mode),
  old_point_(key->bezier_control(mode_)),
  new_point_(point)
{
}

KeyframeSetBezierControlPoint::KeyframeSetBezierControlPoint(NodeKeyframePtr key, NodeKeyframe::BezierType mode, const QPointF &new_point, const QPointF &old_point, QUndoCommand *parent) :
  UndoCommand(parent),
  key_(key),
  mode_(mode),
  old_point_(old_point),
  new_point_(new_point)
{
}

void KeyframeSetBezierControlPoint::redo_internal()
{
  key_->set_bezier_control(mode_, new_point_);
}

void KeyframeSetBezierControlPoint::undo_internal()
{
  key_->set_bezier_control(mode_, old_point_);
}

OLIVE_NAMESPACE_EXIT
