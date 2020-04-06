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

#ifndef KEYFRAMEVIEWUNDO_H
#define KEYFRAMEVIEWUNDO_H

#include "node/keyframe.h"
#include "undo/undocommand.h"

OLIVE_NAMESPACE_ENTER

class KeyframeSetTypeCommand : public UndoCommand {
public:
  KeyframeSetTypeCommand(NodeKeyframePtr key, NodeKeyframe::Type type, QUndoCommand* parent = nullptr);

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  NodeKeyframePtr key_;

  NodeKeyframe::Type old_type_;

  NodeKeyframe::Type new_type_;

};

class KeyframeSetBezierControlPoint : public UndoCommand {
public:
  KeyframeSetBezierControlPoint(NodeKeyframePtr key, NodeKeyframe::BezierType mode, const QPointF& point, QUndoCommand* parent = nullptr);
  KeyframeSetBezierControlPoint(NodeKeyframePtr key, NodeKeyframe::BezierType mode, const QPointF& new_point, const QPointF& old_point, QUndoCommand* parent = nullptr);

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  NodeKeyframePtr key_;

  NodeKeyframe::BezierType mode_;

  QPointF old_point_;

  QPointF new_point_;

};

OLIVE_NAMESPACE_EXIT

#endif // KEYFRAMEVIEWUNDO_H
