#include "nodeblock.h"

NodeBlock::NodeBlock(NodeGraph *graph) :
  Node(graph)
{
  previous_block_ = new NodeIO(this, "prev_block", tr("Previous"), true, false);
  previous_block_->AddAcceptedNodeInput(olive::nodes::kBlock);

  AddParameter(previous_block_);

  next_block_ = new NodeIO(this, "next_block", tr("Next"), true, false);
  previous_block_->SetOutputDataType(olive::nodes::kBlock);
  AddParameter(next_block_);
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

}
