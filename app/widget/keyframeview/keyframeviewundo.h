#ifndef KEYFRAMEVIEWUNDO_H
#define KEYFRAMEVIEWUNDO_H

#include <QUndoCommand>

#include "node/keyframe.h"

class KeyframeSetTypeCommand : public QUndoCommand {
public:
  KeyframeSetTypeCommand(NodeKeyframePtr key, NodeKeyframe::Type type, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  NodeKeyframePtr key_;

  NodeKeyframe::Type old_type_;

  NodeKeyframe::Type new_type_;

};

class KeyframeSetBezierControlPoint : public QUndoCommand {
public:
  KeyframeSetBezierControlPoint(NodeKeyframePtr key, NodeKeyframe::BezierType mode, const QPointF& point, QUndoCommand* parent = nullptr);
  KeyframeSetBezierControlPoint(NodeKeyframePtr key, NodeKeyframe::BezierType mode, const QPointF& new_point, const QPointF& old_point, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  NodeKeyframePtr key_;

  NodeKeyframe::BezierType mode_;

  QPointF old_point_;

  QPointF new_point_;

};

#endif // KEYFRAMEVIEWUNDO_H
