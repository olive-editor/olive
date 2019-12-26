#include "transition.h"

TransitionBlock::TransitionBlock()
{
  out_block_input_ = new NodeInput("out_block_in", NodeParam::kBuffer);
  out_block_input_->set_is_keyframable(false);
  AddInput(out_block_input_);

  in_block_input_ = new NodeInput("in_block_in", NodeParam::kBuffer);
  in_block_input_->set_is_keyframable(false);
  AddInput(in_block_input_);

  // A block's length must be greater than 0
  set_in_and_out_offset(1, 1);
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

void TransitionBlock::set_length_and_media_out(const rational &length)
{
  Q_UNUSED(length)
  qCritical() << "Set length is not permitted on a transition";
  abort();
}

void TransitionBlock::set_length_and_media_in(const rational &length)
{
  Q_UNUSED(length)
  qCritical() << "Set length and media in is not permitted on a transition";
  abort();
}

const rational &TransitionBlock::in_offset() const
{
  return in_offset_;
}

const rational &TransitionBlock::out_offset() const
{
  return out_offset_;
}

void TransitionBlock::set_in_and_out_offset(const rational &in_offset, const rational &out_offset)
{
  in_offset_ = in_offset;
  out_offset_ = out_offset;
  RecalculateLength();
}

void TransitionBlock::RecalculateLength()
{
  Block::set_length_and_media_out(in_offset_ + out_offset_);
}
