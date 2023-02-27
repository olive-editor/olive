/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "timelineundosplit.h"

#include "node/block/clip/clip.h"
#include "node/block/transition/transition.h"
#include "node/nodeundo.h"

namespace olive {

//
// BlockSplitCommand
//
void BlockSplitCommand::prepare()
{
  reconnect_tree_command_ = new MultiUndoCommand();
  new_block_ = static_cast<Block*>(Node::CopyNodeInGraph(block_, reconnect_tree_command_));
}

void BlockSplitCommand::redo()
{
  old_length_ = block_->length();

  Q_ASSERT(point_ > block_->in() && point_ < block_->out());

  reconnect_tree_command_->redo_now();

  // Determine our new lengths
  rational new_length = point_ - block_->in();
  rational new_part_length = block_->out() - point_;

  // Begin an operation
  Track* track = block_->track();

  // Set lengths
  block_->set_length_and_media_out(new_length);
  new_block()->set_length_and_media_in(new_part_length);

  // Insert new block
  track->InsertBlockAfter(new_block(), block_);

  if (ClipBlock *new_clip = dynamic_cast<ClipBlock*>(new_block_)) {
    ClipBlock *old_clip = static_cast<ClipBlock*>(block_);
    new_clip->AddCachePassthroughFrom(old_clip);
  }

  // If the block had an out transition, we move it to the new block
  moved_transition_ = NodeInput();

  TransitionBlock* potential_transition = dynamic_cast<TransitionBlock*>(new_block()->next());
  if (potential_transition) {
    for (const Node::OutputConnection& output : block_->output_connections()) {
      if (output.second.node() == potential_transition) {
        moved_transition_ = NodeInput(potential_transition, TransitionBlock::kOutBlockInput);
        Node::DisconnectEdge(block_, moved_transition_);
        Node::ConnectEdge(new_block(), moved_transition_);
        break;
      }
    }
  }
}

void BlockSplitCommand::undo()
{
  Track* track = block_->track();

  if (moved_transition_.IsValid()) {
    Node::DisconnectEdge(new_block(), moved_transition_);
    Node::ConnectEdge(block_, moved_transition_);
  }

  block_->set_length_and_media_out(old_length_);
  track->RippleRemoveBlock(new_block());

  // If we ran a reconnect command, disconnect now
  reconnect_tree_command_->undo_now();
}

//
// BlockSplitPreservingLinksCommand
//
Block *BlockSplitPreservingLinksCommand::GetSplit(Block *original, int time_index) const
{
  if (time_index >= 0 && time_index < times_.size()) {
    int original_index = blocks_.indexOf(original);
    if (original_index != -1) {
      return splits_.at(time_index).at(original_index);
    }
  }

  return nullptr;
}

void BlockSplitPreservingLinksCommand::prepare()
{
  splits_.resize(times_.size());

  for (int i=0;i<times_.size();i++) {
    const rational& time = times_.at(i);

    // FIXME: I realize this isn't going to work if the times aren't ordered. I'm lazy so rather
    //        than writing in a sorting algorithm here, I'll just put an assert as a reminder
    //        if this ever becomes an issue.
    Q_ASSERT(i == 0 || time > times_.at(i-1));

    QVector<Block*> splits(blocks_.size());

    for (int j=0;j<blocks_.size();j++) {
      Block* b = blocks_.at(j);

      if (b->in() < time && b->out() > time) {
        BlockSplitCommand* split_command = new BlockSplitCommand(b, time);
        split_command->redo_now();
        splits.replace(j, split_command->new_block());
        commands_.append(split_command);
      } else {
        splits.replace(j, nullptr);
      }
    }

    splits_.replace(i, splits);
  }

  // Now that we've determined all the splits, we can relink everything
  for (int i=0;i<blocks_.size();i++) {
    Block* a = blocks_.at(i);

    for (int j=0;j<blocks_.size();j++) {
      if (i == j) {
        continue;
      }

      Block* b = blocks_.at(j);

      if (Block::AreLinked(a, b)) {
        // These blocks are linked, ensure all the splits are linked too

        foreach (const QVector<Block*>& split_list, splits_) {
          NodeLinkCommand* blc = new NodeLinkCommand(split_list.at(i), split_list.at(j), true);
          blc->redo_now();
          commands_.append(blc);
        }
      }
    }
  }
}

//
// TrackSplitAtTimeCommand
//
void TrackSplitAtTimeCommand::prepare()
{
  // Find Block that contains this time
  Block* b = track_->BlockContainingTime(point_);

  if (b) {
    command_ = new BlockSplitCommand(b, point_);
  }
}

}
