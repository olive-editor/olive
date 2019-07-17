#include "nodeviewundo.h"

NodeEdgeAddCommand::NodeEdgeAddCommand(NodeOutput *output, NodeInput *input, QUndoCommand *parent) :
  QUndoCommand(parent),
  output_(output),
  input_(input),
  old_edge_(nullptr),
  done_(false)
{
}

void NodeEdgeAddCommand::redo()
{
  if (done_) {
    return;
  }

  old_edge_ = NodeParam::DisconnectForNewOutput(input_);

  NodeParam::ConnectEdge(output_, input_);

  done_ = true;
}

void NodeEdgeAddCommand::undo()
{
  if (!done_) {
    return;
  }

  NodeParam::DisconnectEdge(output_, input_);

  if (old_edge_ != nullptr) {
    NodeParam::ConnectEdge(old_edge_->output(), old_edge_->input());
  }

  done_ = false;
}

NodeEdgeRemoveCommand::NodeEdgeRemoveCommand(NodeOutput *output, NodeInput *input, QUndoCommand *parent) :
  QUndoCommand(parent),
  output_(output),
  input_(input),
  done_(false)
{
}

void NodeEdgeRemoveCommand::redo()
{
  if (done_) {
    return;
  }

  NodeParam::DisconnectEdge(output_, input_);
  done_ = true;
}

void NodeEdgeRemoveCommand::undo()
{
  if (!done_) {
    return;
  }

  NodeParam::ConnectEdge(output_, input_);
  done_ = false;
}
