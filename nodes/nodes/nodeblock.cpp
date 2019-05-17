#include "nodeblock.h"

NodeBlock::NodeBlock(NodeGraph *graph) :
  Node(graph)
{
  previous_block_ = new NodeIO(this, "prev_block", tr("Previous"), true, false);
  previous_block_->AddAcceptedNodeInput(olive::nodes::kBlock);
  previous_block_->SetValue(0);
  AddParameter(previous_block_);

  next_block_ = new NodeIO(this, "next_block", tr("Next"), true, false);
  next_block_->SetOutputDataType(olive::nodes::kBlock);
  next_block_->SetValue(0);
  AddParameter(next_block_);
}

rational NodeBlock::track_in()
{
  rational in_point;

  NodeBlock* previous = previous_block();
  while (previous != nullptr) {
    in_point += previous->length();
  }

  return in_point;
}

rational NodeBlock::track_out()
{
  return track_in() + length();
}

const rational &NodeBlock::length()
{
  return length_;
}

void NodeBlock::set_length(const rational &l)
{
  length_ = l;
}

QString NodeBlock::name()
{
  return tr("Block");
}

QString NodeBlock::id()
{
  return "block";
}

NodeBlock *NodeBlock::previous_block()
{
  return reinterpret_cast<NodeBlock*>(previous_block_->GetValue().value<quintptr>());
}

void NodeBlock::set_previous_block(NodeBlock *p)
{
  previous_block_->SetValue(reinterpret_cast<quintptr>(p));
}

NodeBlock *NodeBlock::next_block()
{
  return reinterpret_cast<NodeBlock*>(next_block_->GetValue().value<quintptr>());
}

void NodeBlock::set_next_block(NodeBlock *p)
{
  next_block_->SetValue(reinterpret_cast<quintptr>(p));
}
