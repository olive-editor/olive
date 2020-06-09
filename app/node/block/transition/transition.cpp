/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "transition.h"

#include "common/clamp.h"

OLIVE_NAMESPACE_ENTER

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
  Block::Retranslate();

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

void TransitionBlock::Hash(QCryptographicHash &hash, const rational &time) const
{
  double all_prog = GetTotalProgress(time);
  double in_prog = GetInProgress(time);
  double out_prog = GetOutProgress(time);

  hash.addData(reinterpret_cast<const char*>(&all_prog), sizeof(double));
  hash.addData(reinterpret_cast<const char*>(&in_prog), sizeof(double));
  hash.addData(reinterpret_cast<const char*>(&out_prog), sizeof(double));

  if (out_block_input_->is_connected()) {
    out_block_input_->get_connected_node()->Hash(hash, time);
  }

  if (in_block_input_->is_connected()) {
    in_block_input_->get_connected_node()->Hash(hash, time);
  }
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

OLIVE_NAMESPACE_EXIT
