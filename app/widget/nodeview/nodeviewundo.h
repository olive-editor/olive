#ifndef NODEVIEWUNDO_H
#define NODEVIEWUNDO_H

#include <QUndoCommand>

#include "node/graph.h"
#include "node/node.h"
#include "nodeviewitem.h"

/**
 * @brief An undoable commnd for connecting two NodeParams together
 *
 * Can be considered a QUndoCommand wrapper for NodeParam::ConnectEdge()/
 */
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

/**
 * @brief An undoable commnd for disconnecting two NodeParams
 *
 * Can be considered a QUndoCommand wrapper for NodeParam::DisonnectEdge()/
 */
class NodeEdgeRemoveCommand : public QUndoCommand {
public:
  NodeEdgeRemoveCommand(NodeOutput* output, NodeInput* input, QUndoCommand* parent = nullptr);
  NodeEdgeRemoveCommand(NodeEdgePtr edge, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  NodeOutput* output_;
  NodeInput* input_;

  bool done_;
};

class NodeAddCommand : public QUndoCommand {
public:
  NodeAddCommand(NodeGraph* graph, Node* node, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  QObject memory_manager_;

  NodeGraph* graph_;
  Node* node_;
};

class NodeRemoveCommand : public QUndoCommand {
public:
  NodeRemoveCommand(NodeGraph* graph,
                    const QList<Node*>& nodes,
                    QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  QObject memory_manager_;

  NodeGraph* graph_;
  QList<Node*> nodes_;
  QList<NodeEdgePtr> edges_;
};

class NodeRemoveWithExclusiveDeps : public QUndoCommand {
public:
  NodeRemoveWithExclusiveDeps(NodeGraph* graph,
                              Node* node,
                              QUndoCommand* parent = nullptr);

private:
  NodeRemoveCommand* remove_command_;
};

class NodeCopyInputsCommand : public QUndoCommand {
public:
  NodeCopyInputsCommand(Node* src,
                        Node* dest,
                        bool include_connections = true,
                        QUndoCommand* parent = nullptr);

  NodeCopyInputsCommand(Node* src,
                        Node* dest,
                        QUndoCommand* parent = nullptr);

  virtual void redo() override;

private:
  Node* src_;

  Node* dest_;

  bool include_connections_;

};

#endif // NODEVIEWUNDO_H
