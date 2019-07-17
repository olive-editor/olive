#ifndef NODEVIEWUNDO_H
#define NODEVIEWUNDO_H

#include <QUndoCommand>

#include "node/node.h"

class NodeEdgeAddCommand : public QUndoCommand {
public:
  NodeEdgeAddCommand(NodeOutput* output, NodeInput* input, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  NodeOutput* output_;
  NodeInput* input_;

  NodeEdgePtr old_edge_;

  bool done_;
};

class NodeEdgeRemoveCommand : public QUndoCommand {
public:
  NodeEdgeRemoveCommand(NodeOutput* output, NodeInput* input, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  NodeOutput* output_;
  NodeInput* input_;

  bool done_;
};

#endif // NODEVIEWUNDO_H
