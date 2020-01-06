#include "transition.h"

#include "common/clamp.h"

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
  // If no in block is connected, there's no in offset
  if (!connected_in_block()) {
    return 0;
  }

  if (!connected_out_block()) {
    // Assume only an in block is connected, in which case this entire transition length
    return length();
  }

  // Assume both are connected
  return length() + media_in();
}

rational TransitionBlock::out_offset() const
{
  // If no in block is connected, there's no in offset
  if (!connected_out_block()) {
    return 0;
  }

  if (!connected_in_block()) {
    // Assume only an in block is connected, in which case this entire transition length
    return length();
  }

  // Assume both are connected
  return -media_in();
}

Block *TransitionBlock::connected_out_block() const
{
  return connected_out_block_;
}

Block *TransitionBlock::connected_in_block() const
{
  return connected_in_block_;
}

double TransitionBlock::GetTotalProgress(const rational &time) const
{
  return GetInternalTransitionTime(time) / length().toDouble();
}

double TransitionBlock::GetOutProgress(const rational &time) const
{
  if (out_offset() == 0) {
    return 0;
  }

  return clamp(1.0 - (GetInternalTransitionTime(time) / out_offset().toDouble()), 0.0, 1.0);
}

double TransitionBlock::GetInProgress(const rational &time) const
{
  if (in_offset() == 0) {
    return 0;
  }

  return clamp((GetInternalTransitionTime(time) - out_offset().toDouble()) / in_offset().toDouble(), 0.0, 1.0);
}

double TransitionBlock::GetInternalTransitionTime(const rational &time) const
{
  return time.toDouble() - in().toDouble();
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
