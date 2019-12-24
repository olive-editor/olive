#ifndef NODEPARAMVIEWUNDO_H
#define NODEPARAMVIEWUNDO_H

#include <QUndoCommand>

#include "node/input.h"

class NodeParamSetKeyframing : public QUndoCommand {
public:
  NodeParamSetKeyframing(NodeInput* input, bool setting, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  NodeInput* input_;
  bool setting_;
};

#endif // NODEPARAMVIEWUNDO_H
