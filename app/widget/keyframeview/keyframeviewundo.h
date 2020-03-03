#ifndef KEYFRAMEVIEWUNDO_H
#define KEYFRAMEVIEWUNDO_H

#include "node/keyframe.h"
#include "undo/undocommand.h"

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

#endif // KEYFRAMEVIEWUNDO_H
