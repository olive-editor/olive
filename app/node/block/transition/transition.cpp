#include "transition.h"

TransitionBlock::TransitionBlock() :
  connected_out_block_(nullptr),
  connected_in_block_(nullptr)
{
  out_block_input_ = new NodeInput("out_block_in", NodeParam::kBuffer);
  out_block_input_->set_is_keyframable(false);
  AddInput(out_block_input_);

  in_block_input_ = new NodeInput("in_block_in", NodeParam::kBuffer);
  in_block_input_->set_is_keyframable(false);
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
