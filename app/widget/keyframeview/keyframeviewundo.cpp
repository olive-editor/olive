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

#include "keyframeviewundo.h"

#include "node/node.h"
#include "project/item/sequence/sequence.h"

namespace olive {

KeyframeSetTypeCommand::KeyframeSetTypeCommand(NodeKeyframe* key, NodeKeyframe::Type type) :
  key_(key),
  old_type_(key->type()),
  new_type_(type)
{
}

Project *KeyframeSetTypeCommand::GetRelevantProject() const
{
  return key_->parent()->project();
}

void KeyframeSetTypeCommand::redo()
{
  key_->set_type(new_type_);
}

void KeyframeSetTypeCommand::undo()
{
  key_->set_type(old_type_);
}

KeyframeSetBezierControlPoint::KeyframeSetBezierControlPoint(NodeKeyframe* key, NodeKeyframe::BezierType mode, const QPointF& point) :
  key_(key),
  mode_(mode),
  old_point_(key->bezier_control(mode_)),
  new_point_(point)
{
}

KeyframeSetBezierControlPoint::KeyframeSetBezierControlPoint(NodeKeyframe* key, NodeKeyframe::BezierType mode, const QPointF &new_point, const QPointF &old_point) :
  key_(key),
  mode_(mode),
  old_point_(old_point),
  new_point_(new_point)
{
}

Project *KeyframeSetBezierControlPoint::GetRelevantProject() const
{
  return key_->parent()->project();
}

void KeyframeSetBezierControlPoint::redo()
{
  key_->set_bezier_control(mode_, new_point_);
}

void KeyframeSetBezierControlPoint::undo()
{
  key_->set_bezier_control(mode_, old_point_);
}

}
