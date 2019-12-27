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

#endif // KEYFRAMEVIEWUNDO_H
