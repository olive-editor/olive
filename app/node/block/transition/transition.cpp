#include "transition.h"

TransitionBlock::TransitionBlock() :
  connected_out_block_(nullptr),
  connected_in_block_(nullptr)
{
  out_block_input_ = new NodeInput("out_block_in", NodeParam::kBuffer);
  out_block_input_->set_is_keyframable(false);
  connect(out_block_input_, &NodeParam::EdgeAdded, this, &TransitionBlock::BlockConnected);
  connect(out_block_input_, &NodeParam::EdgeRemoved, this, &TransitionBlock::BlockDisconnected);
  AddInput(out_block_input_);

  in_block_input_ = new NodeInput("in_block_in", NodeParam::kBuffer);
  in_block_input_->set_is_keyframable(false);
  connect(in_block_input_, &NodeParam::EdgeAdded, this, &TransitionBlock::BlockConnected);
  connect(in_block_input_, &NodeParam::EdgeRemoved, this, &TransitionBlock::BlockDisconnected);
  AddInput(in_block_input_);
}

Block::Type TransitionBlock::type() const
{
  return kTransition;
}

NodeInput *TransitionBlock::out_block_input() const
{
  return out_block_input_;
}

NodeInput *TransitionBlock::in_block_input() const
{
  return in_block_input_;
}

void TransitionBlock::Retranslate()
{
  out_block_input_->set_name(tr("From"));
  in_block_input_->set_name(tr("To"));
}

rational TransitionBlock::in_offset() const
{
  return 0;
}

rational TransitionBlock::out_offset() const
{
  return 0;
}

void TransitionBlock::BlockConnected(NodeEdgePtr edge)
{
  if (!edge->output()->parentNode()->IsBlock()) {
    return;
  }

  Block* block = static_cast<Block*>(edge->output()->parentNode());

  if (edge->input() == out_block_input_) {
    connected_out_block_ = block;
  } else {
    connected_in_block_ = block;
  }
}

void TransitionBlock::BlockDisconnected(NodeEdgePtr edge)
{
  if (edge->input() == out_block_input_) {
    connected_out_block_ = nullptr;
  } else {
    connected_in_block_ = nullptr;
  }
}
