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

#include "undo.h"

#include "core.h"
#include "node/block/clip/clip.h"
#include "node/block/transition/transition.h"
#include "node/graph.h"
#include "widget/nodeview/nodeviewundo.h"

OLIVE_NAMESPACE_ENTER

Node* TakeNodeFromParentGraph(Node* n, QObject* new_parent = nullptr)
{
  static_cast<NodeGraph*>(n->parent())->TakeNode(n, new_parent);

  return n;
}

BlockResizeCommand::BlockResizeCommand(Block *block, rational new_length, QUndoCommand* parent) :
  UndoCommand(parent),
  block_(block),
  old_length_(block->length()),
  new_length_(new_length)
{
}

Project *BlockResizeCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(block_->parent())->project();
}

void BlockResizeCommand::redo_internal()
{
  block_->set_length_and_media_out(new_length_);
}

void BlockResizeCommand::undo_internal()
{
  block_->set_length_and_media_out(old_length_);
}

BlockResizeWithMediaInCommand::BlockResizeWithMediaInCommand(Block *block, rational new_length, QUndoCommand *parent) :
  UndoCommand(parent),
  block_(block),
  old_length_(block->length()),
  new_length_(new_length)
{
}

Project *BlockResizeWithMediaInCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(block_->parent())->project();
}

void BlockResizeWithMediaInCommand::redo_internal()
{
  block_->set_length_and_media_in(new_length_);
}

void BlockResizeWithMediaInCommand::undo_internal()
{
  block_->set_length_and_media_in(old_length_);
}

BlockSetMediaInCommand::BlockSetMediaInCommand(Block *block, rational new_media_in, QUndoCommand* parent) :
  UndoCommand(parent),
  block_(block),
  old_media_in_(block->media_in()),
  new_media_in_(new_media_in)
{
}

Project *BlockSetMediaInCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(block_->parent())->project();
}

void BlockSetMediaInCommand::redo_internal()
{
  block_->set_media_in(new_media_in_);
}

void BlockSetMediaInCommand::undo_internal()
{
  block_->set_media_in(old_media_in_);
}

TrackRippleRemoveBlockCommand::TrackRippleRemoveBlockCommand(TrackOutput *track, Block *block, QUndoCommand *parent) :
  UndoCommand(parent),
  track_(track),
  block_(block)
{
}

Project *TrackRippleRemoveBlockCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(block_->parent())->project();
}

void TrackRippleRemoveBlockCommand::redo_internal()
{
  before_ = block_->previous();
  track_->RippleRemoveBlock(block_);
}

void TrackRippleRemoveBlockCommand::undo_internal()
{
  if (before_) {
    track_->InsertBlockAfter(block_, before_);
  } else {
    track_->PrependBlock(block_);
  }
}

TrackInsertBlockAfterCommand::TrackInsertBlockAfterCommand(TrackOutput *track,
                                                           Block *block,
                                                           Block *before,
                                                           QUndoCommand *parent) :
  UndoCommand(parent),
  track_(track),
  block_(block),
  before_(before)
{
}

Project *TrackInsertBlockAfterCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(block_->parent())->project();
}

void TrackInsertBlockAfterCommand::redo_internal()
{
  track_->InsertBlockAfter(block_, before_);
}

void TrackInsertBlockAfterCommand::undo_internal()
{
  track_->RippleRemoveBlock(block_);
}

TrackRippleRemoveAreaCommand::TrackRippleRemoveAreaCommand(TrackOutput *track, rational in, rational out, QUndoCommand *parent) :
  UndoCommand(parent),
  track_(track),
  in_(in),
  out_(out),
  splice_(false),
  trim_out_(nullptr),
  trim_in_(nullptr),
  insert_(nullptr)
{
}

Project *TrackRippleRemoveAreaCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(track_->parent())->project();
}

void TrackRippleRemoveAreaCommand::SetInsert(Block *insert)
{
  insert_ = insert;
}

void TrackRippleRemoveAreaCommand::redo_internal()
{
  // Iterate through blocks determining which need trimming/removing/splitting
  foreach (Block* block, track_->Blocks()) {
    if (block->in() < in_ && block->out() > out_) {
      // The area entirely within this Block
      trim_out_ = block;
      splice_ = true;

      // We don't need to do anything else here
      break;
    } else if (block->in() >= in_ && block->out() <= out_) {
      // This Block's is entirely within the area
      removed_blocks_.append(block);
    } else if (block->in() < in_ && block->out() >= in_) {
      // This Block's out point exceeds `in`
      trim_out_ = block;
    } else if (block->in() <= out_ && block->out() > out_) {
      // This Block's in point exceeds `out`
      trim_in_ = block;
    }
  }

  track_->BeginOperation();

  // If we picked up a block to splice
  if (splice_) {

    // Split the block here
    trim_in_ = static_cast<Block*>(trim_out_->copy());

    static_cast<NodeGraph*>(track_->parent())->AddNode(trim_in_);
    Node::CopyInputs(trim_out_, trim_in_);

    trim_out_old_length_ = trim_out_->length();
    trim_out_->set_length_and_media_out(in_ - trim_out_->in());

    trim_in_->set_length_and_media_in(trim_out_old_length_ - (out_ - trim_out_->in()));

    track_->InsertBlockAfter(trim_in_, trim_out_);

  } else {

    // If we picked up a block to trim the in point of
    if (trim_in_) {
      trim_in_old_length_ = trim_in_->length();
      trim_in_new_length_ = trim_in_->out() - out_;
    }

    // If we picked up a block to trim the out point of
    if (trim_out_) {
      trim_out_old_length_ = trim_out_->length();
      trim_out_new_length_ = in_ - trim_out_->in();
    }

    // If we picked up a block to trim the in point of
    if (trim_in_old_length_ != trim_in_new_length_) {
      trim_in_->set_length_and_media_in(trim_in_new_length_);
    }

    // Remove all blocks that are flagged for removal
    foreach (Block* remove_block, removed_blocks_) {
      track_->RippleRemoveBlock(remove_block);

      BlockUnlinkAllCommand* unlink_command = new BlockUnlinkAllCommand(remove_block);
      unlink_command->redo();
      remove_block_commands_.append(unlink_command);

      NodeRemoveWithExclusiveDeps* remove_command = new NodeRemoveWithExclusiveDeps(static_cast<NodeGraph*>(remove_block->parent()), remove_block);
      remove_command->redo();
      remove_block_commands_.append(remove_command);
    }

    // If we picked up a block to trim the out point of
    if (trim_out_old_length_ != trim_out_new_length_) {
      trim_out_->set_length_and_media_out(trim_out_new_length_);
    }

  }

  // If we were given a block to insert, insert it here
  if (insert_) {
    if (!trim_out_) {
      // This is the start of the Sequence
      track_->PrependBlock(insert_);
    } else if (!trim_in_) {
      // This is the end of the Sequence
      track_->AppendBlock(insert_);
    } else {
      // This is somewhere in the middle of the Sequence
      track_->InsertBlockAfter(insert_, trim_out_);
    }
  }

  track_->EndOperation();

  track_->InvalidateCache(TimeRange(in_, insert_ ? out_ : RATIONAL_MAX),
                          track_->block_input(),
                          track_->block_input());
}

void TrackRippleRemoveAreaCommand::undo_internal()
{
  track_->BeginOperation();

  // If we were given a block to insert, insert it here
  if (insert_ != nullptr) {
    track_->RippleRemoveBlock(insert_);
  }

  if (splice_) {

    // trim_in_ is our copy and trim_out_ is our original
    track_->RippleRemoveBlock(trim_in_);
    trim_out_->set_length_and_media_out(trim_out_old_length_);

    TakeNodeFromParentGraph(trim_in_, &memory_manager_);

  } else {

    // If we picked up a block to trim the out point of
    if (trim_out_old_length_ != trim_out_new_length_) {
      trim_out_->set_length_and_media_out(trim_out_old_length_);
    }

    // Restore blocks that were removed
    for (int i=remove_block_commands_.size()-1;i>=0;i--) {
      UndoCommand* command = remove_block_commands_.at(i);
      command->undo();
      delete command;
    }
    remove_block_commands_.clear();

    for (int i=removed_blocks_.size()-1;i>=0;i--) {
      Block* remove_block = removed_blocks_.at(i);

      if (trim_in_) {
        track_->InsertBlockBefore(remove_block, trim_in_);
      } else {
        track_->AppendBlock(remove_block);
      }
    }
    removed_blocks_.clear();

    // If we picked up a block to trim the in point of
    if (trim_in_old_length_ != trim_in_new_length_) {
      trim_in_->set_length_and_media_in(trim_in_old_length_);
    }

  }

  track_->EndOperation();

  track_->InvalidateCache(TimeRange(in_, insert_ ? out_ : RATIONAL_MAX), track_->block_input(), track_->block_input());
}

TrackPlaceBlockCommand::TrackPlaceBlockCommand(TrackList *timeline, int track, Block *block, rational in, QUndoCommand *parent) :
  TrackRippleRemoveAreaCommand(nullptr, in, 0, parent), // Out gets set correctly in redo()
  timeline_(timeline),
  track_index_(track),
  gap_(nullptr)
{
  insert_ = block;
}

Project *TrackPlaceBlockCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(static_cast<ViewerOutput*>(timeline_->parent())->parent())->project();
}

void TrackPlaceBlockCommand::redo_internal()
{
  added_track_count_ = 0;

  // Get track (or make it if necessary)
  while (track_index_ >= timeline_->GetTracks().size()) {
    timeline_->AddTrack();

    added_track_count_++;
  }

  track_ = timeline_->GetTrackAt(track_index_);

  append_ = (in_ >= track_->track_length());

  // Check if the placement location is past the end of the timeline
  if (append_) {
    if (in_ > track_->track_length()) {
      // If so, insert a gap here
      gap_ = new GapBlock();
      gap_->set_length_and_media_out(in_ - track_->track_length());
      static_cast<NodeGraph*>(track_->parent())->AddNode(gap_);
      track_->AppendBlock(gap_);
    }

    track_->AppendBlock(insert_);
  } else {
    out_ = in_ + insert_->length();

    // Place the Block at this point
    TrackRippleRemoveAreaCommand::redo_internal();
  }
}

void TrackPlaceBlockCommand::undo_internal()
{
  if (append_) {
    track_->RippleRemoveBlock(insert_);

    if (gap_ != nullptr) {
      track_->RippleRemoveBlock(gap_);
      TakeNodeFromParentGraph(gap_, &memory_manager_);
    }
  } else {
    TrackRippleRemoveAreaCommand::undo_internal();
  }

  for (;added_track_count_>0;added_track_count_--) {
    timeline_->RemoveTrack(&memory_manager_);
  }
}

BlockSplitCommand::BlockSplitCommand(TrackOutput* track, Block *block, rational point, QUndoCommand *parent) :
  UndoCommand(parent),
  track_(track),
  block_(block),
  new_length_(point - block->in()),
  old_length_(block->length()),
  point_(point)
{
  Q_ASSERT(point > block_->in() && point < block_->out() && block_->type() == Block::kClip);

  // Ensures that this block is deleted if this action is undone
  new_block_ = static_cast<Block*>(block_->copy());
  new_block_->setParent(&memory_manager_);

  // Determine if the block outputs to an "out" transition
  TransitionBlock* transition = TransitionBlock::GetBlockOutTransition(block_);
  if (transition) {
    transitions_to_move_.append(transition->out_block_input());
  }
}

Project *BlockSplitCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(block_->parent())->project();
}

void BlockSplitCommand::redo_internal()
{
  track_->BeginOperation();

  static_cast<NodeGraph*>(block_->parent())->AddNode(new_block_);
  Node::CopyInputs(block_, new_block_);

  rational new_part_length = block_->length() - (point_ - block_->in());

  block_->set_length_and_media_out(new_length_);

  new_block_->set_length_and_media_in(new_part_length);

  track_->InsertBlockAfter(new_block_, block_);

  foreach (NodeInput* transition, transitions_to_move_) {
    NodeParam::DisconnectEdge(block_->output(), transition);
    NodeParam::ConnectEdge(new_block_->output(), transition);
  }

  track_->EndOperation();
}

void BlockSplitCommand::undo_internal()
{
  track_->BeginOperation();

  block_->set_length_and_media_out(old_length_);
  track_->RippleRemoveBlock(new_block_);

  TakeNodeFromParentGraph(new_block_, &memory_manager_);

  foreach (NodeInput* transition, transitions_to_move_) {
    NodeParam::DisconnectEdge(new_block_->output(), transition);
    NodeParam::ConnectEdge(block_->output(), transition);
  }

  track_->EndOperation();
}

Block *BlockSplitCommand::new_block()
{
  return new_block_;
}

TrackSplitAtTimeCommand::TrackSplitAtTimeCommand(TrackOutput *track, rational point, QUndoCommand *parent) :
  UndoCommand(parent),
  track_(track)
{
  // Find Block that contains this time
  foreach (Block* b, track->Blocks()) {
    if (b->out() == point) {
      // This time is between blocks, no split needs to occur
      return;
    } else if (b->in() < point && b->out() > point) {
      // We found the Block, split it
      new BlockSplitCommand(track_, b, point, this);
      return;
    }
  }
}

Project *TrackSplitAtTimeCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(track_->parent())->project();
}

TrackReplaceBlockCommand::TrackReplaceBlockCommand(TrackOutput* track, Block *old, Block *replace, QUndoCommand *parent) :
  UndoCommand(parent),
  track_(track),
  old_(old),
  replace_(replace)
{
}

Project *TrackReplaceBlockCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(track_->parent())->project();
}

void TrackReplaceBlockCommand::redo_internal()
{
  track_->ReplaceBlock(old_, replace_);
}

void TrackReplaceBlockCommand::undo_internal()
{
  track_->ReplaceBlock(replace_, old_);
}

TrackPrependBlockCommand::TrackPrependBlockCommand(TrackOutput *track, Block *block, QUndoCommand *parent) :
  UndoCommand(parent),
  track_(track),
  block_(block)
{
}

Project *TrackPrependBlockCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(track_->parent())->project();
}

void TrackPrependBlockCommand::redo_internal()
{
  track_->PrependBlock(block_);
}

void TrackPrependBlockCommand::undo_internal()
{
  track_->RippleRemoveBlock(block_);
}

BlockSplitPreservingLinksCommand::BlockSplitPreservingLinksCommand(const QVector<Block *> &blocks, const QList<rational> &times, QUndoCommand *parent) :
  UndoCommand(parent),
  blocks_(blocks),
  times_(times)
{
  QVector< QVector<Block*> > split_blocks(times.size());

  for (int i=0;i<times.size();i++) {
    const rational& time = times.at(i);

    QVector<Block*> splits(blocks.size());

    for (int j=0;j<blocks.size();j++) {
      Block* b = blocks.at(j);

      if (b->in() < time && b->out() > time) {
        TrackOutput* track = TrackOutput::TrackFromBlock(b);

        Q_ASSERT(track);

        BlockSplitCommand* split_command = new BlockSplitCommand(track, b, time, this);
        splits.replace(j, split_command->new_block());
      } else {
        splits.replace(j, nullptr);
      }
    }

    split_blocks.replace(i, splits);
  }

  // Now that we've determined all the splits, we can relink everything
  for (int i=0;i<blocks_.size();i++) {
    Block* a = blocks.at(i);

    for (int j=0;j<blocks.size();j++) {
      if (i == j) {
        continue;
      }

      Block* b = blocks.at(j);

      if (Block::AreLinked(a, b)) {
        // These blocks are linked, ensure all the splits are linked too

        foreach (const QVector<Block*>& split_list, split_blocks) {
          Block::Link(split_list.at(i), split_list.at(j));
        }
      }
    }
  }
}

Project *BlockSplitPreservingLinksCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(blocks_.first()->parent())->project();
}

TimelineRippleDeleteGapsAtRegionsCommand::TimelineRippleDeleteGapsAtRegionsCommand(ViewerOutput *vo, const TimeRangeList &regions, QUndoCommand *parent) :
  UndoCommand(parent),
  timeline_(vo),
  regions_(regions)
{
}

Project *TimelineRippleDeleteGapsAtRegionsCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(timeline_->parent())->project();
}

void TimelineRippleDeleteGapsAtRegionsCommand::redo_internal()
{
  foreach (const TimeRange& range, regions_) {
    rational max_ripple_length = range.length();

    QList<Block*> blocks_around_range;

    foreach (TrackOutput* track, timeline_->GetTracks()) {
      // Get the block from every other track that is either at or just before our block's in point
      Block* block_at_time = track->NearestBlockBeforeOrAt(range.in());

      if (block_at_time) {
        if (block_at_time->type() == Block::kGap) {
          max_ripple_length = qMin(block_at_time->length(), max_ripple_length);
        } else {
          max_ripple_length = 0;
          break;
        }

        blocks_around_range.append(block_at_time);
      }
    }

    if (max_ripple_length > 0) {
      foreach (Block* resize, blocks_around_range) {
        if (resize->length() == max_ripple_length) {
          // Remove block entirely
          TrackRippleRemoveBlockCommand* remove_command = new TrackRippleRemoveBlockCommand(TrackOutput::TrackFromBlock(resize), resize);
          remove_command->redo();
          commands_.append(remove_command);
        } else {
          // Resize block
          BlockResizeCommand* resize_command = new BlockResizeCommand(resize, resize->length() - max_ripple_length);
          resize_command->redo();
          commands_.append(resize_command);
        }
      }
    }
  }
}

void TimelineRippleDeleteGapsAtRegionsCommand::undo_internal()
{
  for (int i=commands_.size()-1;i>=0;i--) {
    commands_.at(i)->undo();
    delete commands_.at(i);
  }
  commands_.empty();
}

WorkareaSetEnabledCommand::WorkareaSetEnabledCommand(Project* project, TimelinePoints *points, bool enabled, QUndoCommand *parent) :
  UndoCommand(parent),
  project_(project),
  points_(points),
  old_enabled_(points_->workarea()->enabled()),
  new_enabled_(enabled)
{
}

Project *WorkareaSetEnabledCommand::GetRelevantProject() const
{
  return project_;
}

void WorkareaSetEnabledCommand::redo_internal()
{
  points_->workarea()->set_enabled(new_enabled_);
}

void WorkareaSetEnabledCommand::undo_internal()
{
  points_->workarea()->set_enabled(old_enabled_);
}

WorkareaSetRangeCommand::WorkareaSetRangeCommand(Project* project, TimelinePoints *points, const TimeRange &range, QUndoCommand *parent) :
  UndoCommand(parent),
  project_(project),
  points_(points),
  old_range_(points_->workarea()->range()),
  new_range_(range)
{
}

Project *WorkareaSetRangeCommand::GetRelevantProject() const
{
  return project_;
}

void WorkareaSetRangeCommand::redo_internal()
{
  points_->workarea()->set_range(new_range_);
}

void WorkareaSetRangeCommand::undo_internal()
{
  points_->workarea()->set_range(old_range_);
}

BlockLinkCommand::BlockLinkCommand(Block *a, Block *b, bool link, QUndoCommand *parent) :
  UndoCommand(parent),
  a_(a),
  b_(b),
  link_(link)
{
}

Project *BlockLinkCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(a_->parent())->project();
}

void BlockLinkCommand::redo_internal()
{
  if (link_) {
    done_ = Block::Link(a_, b_);
  } else {
    done_ = Block::Unlink(a_, b_);
  }
}

void BlockLinkCommand::undo_internal()
{
  if (done_) {
    if (link_) {
      Block::Unlink(a_, b_);
    } else {
      Block::Link(a_, b_);
    }
  }
}

BlockUnlinkAllCommand::BlockUnlinkAllCommand(Block *block, QUndoCommand *parent) :
  UndoCommand(parent),
  block_(block)
{
}

Project *BlockUnlinkAllCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(block_->parent())->project();
}

void BlockUnlinkAllCommand::redo_internal()
{
  unlinked_ = block_->linked_clips();

  foreach (Block* link, unlinked_) {
    Block::Unlink(block_, link);
  }
}

void BlockUnlinkAllCommand::undo_internal()
{
  foreach (Block* link, unlinked_) {
    Block::Link(block_, link);
  }

  unlinked_.clear();
}

BlockLinkManyCommand::BlockLinkManyCommand(const QList<Block *> blocks, bool link, QUndoCommand *parent) :
  UndoCommand(parent),
  blocks_(blocks)
{
  foreach (Block* a, blocks_) {
    foreach (Block* b, blocks_) {
      if (a != b) {
        new BlockLinkCommand(a, b, link, this);
      }
    }
  }
}

Project *BlockLinkManyCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(blocks_.first()->parent())->project();
}

BlockEnableDisableCommand::BlockEnableDisableCommand(Block *block, bool enabled, QUndoCommand *parent) :
  UndoCommand(parent),
  block_(block),
  old_enabled_(block_->is_enabled()),
  new_enabled_(enabled)
{
}

Project *BlockEnableDisableCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(block_->parent())->project();
}

void BlockEnableDisableCommand::redo_internal()
{
  block_->set_enabled(new_enabled_);
}

void BlockEnableDisableCommand::undo_internal()
{
  block_->set_enabled(old_enabled_);
}

BlockTrimCommand::BlockTrimCommand(TrackOutput* track, Block *block, rational new_length, Timeline::MovementMode mode, QUndoCommand *command) :
  UndoCommand(command),
  track_(track),
  block_(block),
  old_length_(block->length()),
  new_length_(new_length),
  mode_(mode),
  adjacent_(nullptr),
  we_created_adjacent_(false),
  we_deleted_adjacent_(false),
  trim_is_a_roll_edit_(false)
{
}

Project *BlockTrimCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(block_->parent())->project();
}

void BlockTrimCommand::redo_internal()
{
  track_->BeginOperation();

  // Will be POSITIVE if trimming shorter and NEGATIVE if trimming longer
  rational trim_diff = old_length_ - new_length_;

  TimeRange invalidate_range;

  if (mode_ == Timeline::kTrimIn) {
    invalidate_range = TimeRange(block_->in(), block_->in() + trim_diff);
    block_->set_length_and_media_in(new_length_);
    adjacent_ = block_->previous();
  } else {
    invalidate_range = TimeRange(block_->out(), block_->out() - trim_diff);
    block_->set_length_and_media_out(new_length_);
    adjacent_ = block_->next();
  }

  if (trim_diff > rational()) {
    // If trimming SHORTER, we'll need to create/modify a gap
    if (adjacent_ && (adjacent_->type() == Block::kGap || trim_is_a_roll_edit_)) {

      // A gap (or equivalent) exists, simply increase the size of it
      if (mode_ == Timeline::kTrimIn) {
        adjacent_->set_length_and_media_out(adjacent_->length() + trim_diff);
      } else {
        adjacent_->set_length_and_media_in(adjacent_->length() + trim_diff);
      }

    } else {

      // Don't create a gap if the trim was at the end of the sequence (which would be indicated by
      // the mode being "trim out" and "block_->next()" being null.
      if (mode_ == Timeline::kTrimIn || block_->next()) {
        // We must create a gap
        we_created_adjacent_ = true;

        adjacent_ = new GapBlock();
        adjacent_->set_length_and_media_out(trim_diff);
        static_cast<NodeGraph*>(track_->parent())->AddNode(adjacent_);

        if (mode_ == Timeline::kTrimIn) {
          track_->InsertBlockBefore(adjacent_, block_);
        } else {
          track_->InsertBlockAfter(adjacent_, block_);
        }
      }

    }
  } else {
    if (adjacent_) {
      // If trimming LONGER, we'll need to trim the adjacent
      // (assume if there's no adjacent, we're at the end of the timeline and do nothing)
      rational adjacent_length = adjacent_->length() + trim_diff;

      if (adjacent_length.isNull()) {
        // Ripple remove block
        track_->RippleRemoveBlock(adjacent_);
        TakeNodeFromParentGraph(adjacent_, &memory_manager_);
        we_deleted_adjacent_ = true;
      } else if (mode_ == Timeline::kTrimIn) {
        adjacent_->set_length_and_media_out(adjacent_length);
      } else {
        adjacent_->set_length_and_media_in(adjacent_length);
      }
    }
  }

  track_->EndOperation();

  if (block_->type() == Block::kTransition) {
    // Whole transition needs to be invalidated
    invalidate_range = TimeRange(block_->in(), block_->out());
  }

  track_->InvalidateCache(invalidate_range, track_->block_input(), track_->block_input());
}

void BlockTrimCommand::undo_internal()
{
  TimeRange invalidate_range;

  if (block_->type() == Block::kTransition) {
    // Whole transition needs to be invalidated
    invalidate_range = TimeRange(block_->in(), block_->out());
  }

  track_->BeginOperation();

  // Will be POSITIVE if trimming shorter and NEGATIVE if trimming longer
  rational trim_diff = old_length_ - new_length_;

  if (trim_diff > rational()) {
    // If trimmed SHORTER, we need to unadjust the gap
    if (we_created_adjacent_) {
      // If we created a gap, just remove it straight up
      track_->RippleRemoveBlock(adjacent_);
      TakeNodeFromParentGraph(adjacent_, &memory_manager_);
      adjacent_ = nullptr;
      we_created_adjacent_ = false;
    } else if (adjacent_) {
      // If we adjusted an existing gap, unadjust here
      adjacent_->set_length_and_media_out(adjacent_->length() - trim_diff);
    }
  } else {
    if (adjacent_) {
      // If trimmed LONGER, we adjusted an existing block
      // (assume if there's no adjacent, we're at the end of the timeline and do nothing)
      if (we_deleted_adjacent_) {
        static_cast<NodeGraph*>(track_->parent())->AddNode(adjacent_);

        if (mode_ == Timeline::kTrimIn) {
          track_->InsertBlockBefore(adjacent_, block_);
        } else {
          track_->InsertBlockAfter(adjacent_, block_);
        }

        we_deleted_adjacent_ = false;
      } else if (mode_ == Timeline::kTrimIn) {
        adjacent_->set_length_and_media_out(adjacent_->length() - trim_diff);
      } else {
        adjacent_->set_length_and_media_in(adjacent_->length() - trim_diff);
      }
    }
  }

  if (mode_ == Timeline::kTrimIn) {
    block_->set_length_and_media_in(old_length_);

    if (block_->type() != Block::kTransition) {
      invalidate_range = TimeRange(block_->in(), block_->in() + trim_diff);
    }
  } else {
    block_->set_length_and_media_out(old_length_);

    if (block_->type() != Block::kTransition) {
      invalidate_range = TimeRange(block_->out(), block_->out() - trim_diff);
    }
  }

  track_->EndOperation();

  track_->InvalidateCache(invalidate_range, track_->block_input(), track_->block_input());
}

TrackReplaceBlockWithGapCommand::TrackReplaceBlockWithGapCommand(TrackOutput *track, Block *block, QUndoCommand *command) :
  UndoCommand(command),
  track_(track),
  block_(block),
  existing_gap_(nullptr),
  existing_merged_gap_(nullptr),
  our_gap_(nullptr)
{
}

Project *TrackReplaceBlockWithGapCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(block_->parent())->project();
}

void TrackReplaceBlockWithGapCommand::redo_internal()
{
  track_->BeginOperation();

  // Invalidate the range inhabited by this block
  TimeRange invalidate_range(block_->in(), block_->out());

  if (block_->next()) {
    // Block has a next, which means it's NOT at the end of the sequence and thus requires a gap
    rational new_gap_length = block_->length();

    Block* previous = block_->previous();
    Block* next = block_->next();

    bool previous_is_a_gap = (previous && previous->type() == Block::kGap);
    bool next_is_a_gap = (next && next->type() == Block::kGap);

    if (previous_is_a_gap && next_is_a_gap) {
      // Clip is preceded and followed by a gap, so we'll merge the two
      existing_gap_ = static_cast<GapBlock*>(previous);

      existing_merged_gap_ = static_cast<GapBlock*>(next);
      new_gap_length += existing_merged_gap_->length();
      track_->RippleRemoveBlock(existing_merged_gap_);
      TakeNodeFromParentGraph(existing_merged_gap_, &memory_manager_);
    } else if (previous_is_a_gap) {
      // Extend this gap to fill space left by block
      existing_gap_ = static_cast<GapBlock*>(previous);
    } else if (next_is_a_gap) {
      // Extend this gap to fill space left by block
      existing_gap_ = static_cast<GapBlock*>(next);
    }

    if (existing_gap_) {
      // Extend an existing gap
      new_gap_length += existing_gap_->length();
      existing_gap_->set_length_and_media_out(new_gap_length);
      track_->RippleRemoveBlock(block_);

      existing_gap_precedes_ = (existing_gap_ == previous);
    } else {
      // No gap exists to fill this space, create a new one and swap it in
      our_gap_ = new GapBlock();
      our_gap_->set_length_and_media_out(new_gap_length);
      static_cast<NodeGraph*>(track_->parent())->AddNode(our_gap_);
      track_->ReplaceBlock(block_, our_gap_);
    }

  } else {
    // Block is at the end of the track, simply remove it

    // Determine if it's proceeded by a gap, and remove that gap if so
    Block* preceding = block_->previous();
    if (preceding && preceding->type() == Block::kGap) {
      track_->RippleRemoveBlock(preceding);
      TakeNodeFromParentGraph(preceding, &memory_manager_);

      existing_merged_gap_ = static_cast<GapBlock*>(preceding);
    }

    // Remove block in question
    track_->RippleRemoveBlock(block_);
  }

  track_->EndOperation();

  track_->InvalidateCache(invalidate_range, track_->block_input(), track_->block_input());
}

void TrackReplaceBlockWithGapCommand::undo_internal()
{
  track_->BeginOperation();

  if (our_gap_) {

    // We made this gap, simply swap our gap back
    track_->ReplaceBlock(our_gap_, block_);
    TakeNodeFromParentGraph(our_gap_, &memory_manager_);
    our_gap_ = nullptr;

  } else if (existing_gap_) {

    // If we're here, assume that we extended an existing gap
    rational original_gap_length = existing_gap_->length() - block_->length();

    // If we merged two gaps together, restore the second one now
    if (existing_merged_gap_) {
      original_gap_length -= existing_merged_gap_->length();
      static_cast<NodeGraph*>(track_->parent())->AddNode(existing_merged_gap_);
      track_->InsertBlockAfter(existing_merged_gap_, existing_gap_);
      existing_merged_gap_ = nullptr;
    }

    // Restore original block
    if (existing_gap_precedes_) {
      track_->InsertBlockAfter(block_, existing_gap_);
    } else {
      track_->InsertBlockBefore(block_, existing_gap_);
    }

    // Restore gap's original length
    existing_gap_->set_length_and_media_out(original_gap_length);

    existing_gap_ = nullptr;

  } else {

    // Our gap and existing gap were both null, our block must have been at the end and thus
    // required no gap extension/replacement

    // However, we may have removed an unnecessary gap that preceded it
     if (existing_merged_gap_) {
       static_cast<NodeGraph*>(track_->parent())->AddNode(existing_merged_gap_);
       track_->AppendBlock(existing_merged_gap_);
       existing_merged_gap_ = nullptr;
     }

     // Restore block
     track_->AppendBlock(block_);

  }

  track_->EndOperation();

  track_->InvalidateCache(TimeRange(block_->in(), block_->out()), track_->block_input(), track_->block_input());
}

TrackSlideCommand::TrackSlideCommand(TrackOutput* track, const QList<Block*>& moving_blocks, Block *in_adjacent, Block *out_adjacent, const rational& movement, QUndoCommand* parent) :
  UndoCommand(parent),
  track_(track),
  blocks_(moving_blocks),
  movement_(movement),
  we_created_in_adjacent_(false),
  in_adjacent_(in_adjacent),
  we_created_out_adjacent_(false),
  out_adjacent_(out_adjacent)
{
  Q_ASSERT(!movement_.isNull());
}

Project *TrackSlideCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(track_->parent())->project();
}

void TrackSlideCommand::redo_internal()
{
  slide_internal(false);
}

void TrackSlideCommand::undo_internal()
{
  slide_internal(true);
}

void TrackSlideCommand::slide_internal(bool undo)
{
  // Make sure all movement blocks' old positions are invalidated
  TimeRange invalidate_range(blocks_.first()->in(), blocks_.last()->out());

  track_->BeginOperation();

  if (undo) {

    // Undo code
    if (we_created_in_adjacent_) {
      // This is a gap we made, we can just delete it entirely
      track_->RippleRemoveBlock(in_adjacent_);
      TakeNodeFromParentGraph(in_adjacent_, &memory_manager_);
      we_created_in_adjacent_ = false;
      in_adjacent_ = nullptr;
    } else if (in_adjacent_->parent() == &memory_manager_) {
      // This is a gap we removed, we can re-insert it now
      static_cast<NodeGraph*>(track_->parent())->AddNode(in_adjacent_);
      track_->InsertBlockBefore(in_adjacent_, blocks_.first());
    } else {
      // We must have just resized this block
      in_adjacent_->set_length_and_media_out(in_adjacent_->length() - movement_);
    }

    if (we_created_out_adjacent_) {
      // This is a gap we made, we can just delete it entirely
      track_->RippleRemoveBlock(out_adjacent_);
      TakeNodeFromParentGraph(out_adjacent_, &memory_manager_);
      we_created_out_adjacent_ = false;
      out_adjacent_ = nullptr;
    } else if (out_adjacent_) {
      // We may not have created an out adjacent if this was the last clip in the track so we have
      // to check here
      if (out_adjacent_->parent() == &memory_manager_) {
        // This is a gap we removed, we can re-insert it now
        static_cast<NodeGraph*>(track_->parent())->AddNode(out_adjacent_);
        track_->InsertBlockAfter(out_adjacent_, blocks_.last());
      } else {
        // We must have just resized this block
        out_adjacent_->set_length_and_media_in(out_adjacent_->length() + movement_);
      }
    }

  } else {

    // Redo code
    if (!in_adjacent_) {
      // For any slide operation to have occurred at all with no in_adjacent, a gap will need to
      // be created
      GapBlock* gap = new GapBlock();
      gap->set_length_and_media_out(movement_);
      static_cast<NodeGraph*>(track_->parent())->AddNode(gap);
      track_->InsertBlockBefore(gap, blocks_.first());
      we_created_in_adjacent_ = true;
      in_adjacent_ = gap;
    } else if (-movement_ == in_adjacent_->length()) {
      // Remove in adjacent entirely
      track_->RippleRemoveBlock(in_adjacent_);
      TakeNodeFromParentGraph(in_adjacent_, &memory_manager_);
    } else {
      // Resize in adjacent
      in_adjacent_->set_length_and_media_out(in_adjacent_->length() + movement_);
    }

    if (!out_adjacent_) {
      // For any slide operation to have occurred at all with no out_adjacent, a gap will need to
      // be created UNLESS this is at the end of the track already
      if (blocks_.last()->next()) {
        GapBlock* gap = new GapBlock();
        gap->set_length_and_media_out(-movement_);
        static_cast<NodeGraph*>(track_->parent())->AddNode(gap);
        track_->InsertBlockAfter(gap, blocks_.last());
        we_created_out_adjacent_ = true;
        out_adjacent_ = gap;
      }
    } else if (movement_ == out_adjacent_->length()) {
      // Remove out adjacent entirely
      track_->RippleRemoveBlock(out_adjacent_);
      TakeNodeFromParentGraph(out_adjacent_, &memory_manager_);
    } else {
      // Resize out adjacent
      out_adjacent_->set_length_and_media_in(out_adjacent_->length() - movement_);
    }

  }

  track_->EndOperation();

  // Make sure all movement blocks' new positions are invalidated
  invalidate_range.set_range(qMin(invalidate_range.in(), blocks_.first()->in()),
                             qMax(invalidate_range.out(), blocks_.last()->out()));

  track_->InvalidateCache(invalidate_range, track_->block_input(), track_->block_input());
}

TrackListRippleRemoveAreaCommand::TrackListRippleRemoveAreaCommand(TrackList *list, rational in, rational out, QUndoCommand *parent) :
  UndoCommand(parent),
  list_(list),
  in_(in),
  out_(out)
{
  all_tracks_unlocked_ = true;

  foreach (TrackOutput* track, list_->GetTracks()) {
    if (track->IsLocked()) {
      all_tracks_unlocked_ = false;
      continue;
    }

    TrackRippleRemoveAreaCommand* c = new TrackRippleRemoveAreaCommand(track, in, out);
    commands_.append(c);
    working_tracks_.append(track);
  }
}

TrackListRippleRemoveAreaCommand::~TrackListRippleRemoveAreaCommand()
{
  qDeleteAll(commands_);
}

Project *TrackListRippleRemoveAreaCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(static_cast<ViewerOutput*>(list_->parent())->parent())->project();
}

void TrackListRippleRemoveAreaCommand::redo_internal()
{
  if (all_tracks_unlocked_) {
    // We can optimize here by simply shifting the whole cache forward instead of re-caching
    // everything following this time
    if (list_->type() == Timeline::kTrackTypeVideo) {
      static_cast<ViewerOutput*>(list_->parent())->ShiftVideoCache(out_, in_);
    } else if (list_->type() == Timeline::kTrackTypeAudio) {
      static_cast<ViewerOutput*>(list_->parent())->ShiftAudioCache(out_, in_);
    }

    foreach (TrackOutput* track, working_tracks_) {
      track->BeginOperation();
    }
  }

  foreach (TrackRippleRemoveAreaCommand* c, commands_) {
    c->redo();
  }

  if (all_tracks_unlocked_) {
    foreach (TrackOutput* track, working_tracks_) {
      track->EndOperation();
    }
  }
}

void TrackListRippleRemoveAreaCommand::undo_internal()
{
  if (all_tracks_unlocked_) {
    // We can optimize here by simply shifting the whole cache forward instead of re-caching
    // everything following this time
    if (list_->type() == Timeline::kTrackTypeVideo) {
      static_cast<ViewerOutput*>(list_->parent())->ShiftVideoCache(in_, out_);
    } else if (list_->type() == Timeline::kTrackTypeAudio) {
      static_cast<ViewerOutput*>(list_->parent())->ShiftAudioCache(in_, out_);
    }

    foreach (TrackOutput* track, working_tracks_) {
      track->BeginOperation();
    }
  }

  foreach (TrackRippleRemoveAreaCommand* c, commands_) {
    c->undo();
  }

  if (all_tracks_unlocked_) {
    foreach (TrackOutput* track, working_tracks_) {
      track->EndOperation();
    }
  }
}

TimelineRippleRemoveAreaCommand::TimelineRippleRemoveAreaCommand(ViewerOutput *timeline, rational in, rational out, QUndoCommand *parent) :
  UndoCommand(parent),
  timeline_(timeline)
{
  for (int i=0; i<Timeline::kTrackTypeCount; i++) {
    new TrackListRippleRemoveAreaCommand(timeline->track_list(static_cast<Timeline::TrackType>(i)),
                                         in,
                                         out,
                                         this);
  }
}

Project *TimelineRippleRemoveAreaCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(timeline_->parent())->project();
}

TrackListRippleToolCommand::TrackListRippleToolCommand(TrackList *track_list, const QList<RippleInfo> &info, const Timeline::MovementMode &movement_mode, QUndoCommand *parent) :
  UndoCommand(parent),
  track_list_(track_list),
  info_(info),
  movement_mode_(movement_mode)
{
  working_data_.resize(info_.size());

  all_tracks_unlocked_ = (info_.size() == track_list_->GetTrackCount());
}

Project *TrackListRippleToolCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(static_cast<ViewerOutput*>(track_list_->parent())->parent())->project();
}

void TrackListRippleToolCommand::redo_internal()
{
  rational old_latest_pt;
  rational earliest_pt;

  if (all_tracks_unlocked_) {
    // We can do some optimization here
    foreach (const RippleInfo& info, info_) {
      info.track->BeginOperation();
    }

    old_latest_pt = RATIONAL_MIN;
    earliest_pt = RATIONAL_MAX;
    foreach (const RippleInfo& info, info_) {
      if (info.block) {
        old_latest_pt = qMax(old_latest_pt, info.block->out());

        if (movement_mode_ == Timeline::kTrimIn) {
          earliest_pt = qMin(earliest_pt, info.block->in());
        } else {
          earliest_pt = qMin(earliest_pt, info.block->out());
        }
      } else {
        old_latest_pt = qMax(old_latest_pt, info.ref_block->out());
        earliest_pt = qMin(earliest_pt, info.ref_block->out());
      }
    }
  }

  for (int i=0;i<info_.size();i++) {
    const RippleInfo& info = info_.at(i);

    Block* b = info.block;

    if (b) {
      // This was a Block that already existed
      if (info.new_length > 0) {
        if (movement_mode_ == Timeline::kTrimIn) {
          // We'll need to shift the media in point too
          b->set_length_and_media_in(info.new_length);
        } else {
          b->set_length_and_media_out(info.new_length);
        }
      } else {
        // Assume the Block was a Gap and it was reduced to zero length, remove it here
        working_data_[i].removed_gap_after = b->previous();
        info.track->RippleRemoveBlock(b);
        TakeNodeFromParentGraph(b, &memory_manager_);
      }
    } else if (info.new_length > 0) {
      // This is a gap we are creating
      GapBlock* gap = new GapBlock();
      gap->set_length_and_media_out(info.new_length);
      static_cast<NodeGraph*>(info.ref_block->parent())->AddNode(gap);
      working_data_[i].created_gap = gap;

      info.track->InsertBlockAfter(gap, info.ref_block);
    }
  }

  if (all_tracks_unlocked_) {
    // We can do some optimization here

    rational new_latest_pt = RATIONAL_MIN;
    for (int i=0;i<info_.size();i++) {
      const RippleInfo& info = info_.at(i);

      if (info.block) {
        if (info.new_length > 0) {
          new_latest_pt = qMax(new_latest_pt, info.block->out());
        } else {
          new_latest_pt = qMax(new_latest_pt, info.block->in());
        }
      } else if (info.new_length > 0) {
        new_latest_pt = qMax(new_latest_pt, working_data_.at(i).created_gap->out());
      }
    }

    if (track_list_->type() == Timeline::kTrackTypeVideo) {
      static_cast<ViewerOutput*>(track_list_->parent())->ShiftVideoCache(old_latest_pt, new_latest_pt);
    } else if (track_list_->type() == Timeline::kTrackTypeAudio) {
      static_cast<ViewerOutput*>(track_list_->parent())->ShiftAudioCache(old_latest_pt, new_latest_pt);
    }

    foreach (const RippleInfo& info, info_) {
      info.track->EndOperation();

      // FIXME: Untested, is this desirable behavior?
      if (earliest_pt < new_latest_pt) {
        info.track->InvalidateCache(TimeRange(earliest_pt, new_latest_pt),
                                    info.track->block_input(),
                                    info.track->block_input());
      }
    }
  }
}

void TrackListRippleToolCommand::undo_internal()
{
  // FIXME: Add cache shift optimization

  // Clean created gaps
  for (int i=info_.size()-1; i>=0; i--) {
    const RippleInfo& info = info_.at(i);

    Block* b = info.block;

    if (b) {
      // This was a Block that already existed
      if (info.new_length > 0) {
        if (movement_mode_ == Timeline::kTrimIn) {
          // We'll need to shift the media in point too
          b->set_length_and_media_in(info.old_length);
        } else {
          b->set_length_and_media_out(info.old_length);
        }
      } else {
        // Assume the Block was a Gap and it was reduced to zero length, remove it here
        Block* previous_block = working_data_[i].removed_gap_after;

        static_cast<NodeGraph*>(info.track->parent())->AddNode(b);

        if (previous_block) {
          info.track->InsertBlockAfter(b, previous_block);
        } else {
          info.track->PrependBlock(b);
        }
      }
    } else if (info.new_length > 0) {
      // We created a gap here, remove it
      GapBlock* gap = working_data_.at(i).created_gap;

      info.track->RippleRemoveBlock(gap);
      TakeNodeFromParentGraph(gap, &memory_manager_);
    }
  }
}

TrackListInsertGaps::TrackListInsertGaps(TrackList *track_list, const rational &point, const rational &length, QUndoCommand *parent) :
  UndoCommand(parent),
  track_list_(track_list),
  point_(point),
  length_(length),
  split_command_(nullptr)
{
  all_tracks_unlocked_ = true;

  foreach (TrackOutput* track, track_list_->GetTracks()) {
    if (track->IsLocked()) {
      all_tracks_unlocked_ = false;
      continue;
    }

    working_tracks_.append(track);
  }
}

Project *TrackListInsertGaps::GetRelevantProject() const
{
  return static_cast<Sequence*>(static_cast<ViewerOutput*>(track_list_->parent())->parent())->project();
}

void TrackListInsertGaps::redo_internal()
{
  if (all_tracks_unlocked_) {
    // Optimize by shifting over since we have a constant amount of time being inserted
    if (track_list_->type() == Timeline::kTrackTypeVideo) {
      static_cast<ViewerOutput*>(track_list_->parent())->ShiftVideoCache(point_, point_ + length_);
    } else if (track_list_->type() == Timeline::kTrackTypeAudio) {
      static_cast<ViewerOutput*>(track_list_->parent())->ShiftAudioCache(point_, point_ + length_);
    }

    foreach (TrackOutput* track, working_tracks_) {
      track->BeginOperation();
    }
  }

  QVector<Block*> blocks_to_split;
  QList<Block*> blocks_to_append_gap_to;

  foreach (TrackOutput* track, working_tracks_) {
    foreach (Block* b, track->Blocks()) {
      if (b->type() == Block::kGap && b->in() <= point_ && b->out() >= point_) {
        gaps_to_extend_.append(b);
      } else if (b->type() == Block::kClip && b->out() >= point_) {
        if (b->out() > point_) {
          blocks_to_split.append(b);
        }

        blocks_to_append_gap_to.append(b);
      }
    }
  }

  foreach (Block* gap, gaps_to_extend_) {
    gap->set_length_and_media_out(gap->length() + length_);
  }

  if (!blocks_to_split.isEmpty()) {
    split_command_ = new BlockSplitPreservingLinksCommand(blocks_to_split, {point_});
    split_command_->redo();
  }

  foreach (Block* block, blocks_to_append_gap_to) {
    GapBlock* gap = new GapBlock();
    gap->set_length_and_media_out(length_);
    static_cast<NodeGraph*>(block->parent())->AddNode(gap);
    TrackOutput::TrackFromBlock(block)->InsertBlockAfter(gap, block);
    gaps_added_.append(gap);
  }

  if (all_tracks_unlocked_) {
    foreach (TrackOutput* track, working_tracks_) {
      track->EndOperation();
    }
  }
}

void TrackListInsertGaps::undo_internal()
{
  if (all_tracks_unlocked_) {
    // Optimize by shifting over since we have a constant amount of time being inserted
    if (track_list_->type() == Timeline::kTrackTypeVideo) {
      static_cast<ViewerOutput*>(track_list_->parent())->ShiftVideoCache(point_ + length_, point_);
    } else if (track_list_->type() == Timeline::kTrackTypeAudio) {
      static_cast<ViewerOutput*>(track_list_->parent())->ShiftAudioCache(point_ + length_, point_);
    }

    foreach (TrackOutput* track, working_tracks_) {
      track->BeginOperation();
    }
  }

  // Remove added gaps
  foreach (GapBlock* gap, gaps_added_) {
    TrackOutput::TrackFromBlock(gap)->RippleRemoveBlock(gap);
    TakeNodeFromParentGraph(gap, &memory_manager_);
  }
  gaps_added_.clear();

  // Un-split blocks
  if (split_command_) {
    split_command_->undo();
    delete split_command_;
    split_command_ = nullptr;
  }

  // Restore original length of gaps
  foreach (Block* gap, gaps_to_extend_) {
    gap->set_length_and_media_out(gap->length() - length_);
  }
  gaps_to_extend_.clear();

  if (all_tracks_unlocked_) {
    foreach (TrackOutput* track, working_tracks_) {
      track->EndOperation();
    }
  }
}

TransitionRemoveCommand::TransitionRemoveCommand(TrackOutput* track, TransitionBlock *block, QUndoCommand* parent) :
  UndoCommand(parent),
  track_(track),
  block_(block),
  out_block_(block_->connected_out_block()),
  in_block_(block_->connected_in_block())
{
  // Can't remove a transition in this way unless it's connected to at least one other block
  Q_ASSERT(out_block_ || in_block_);
}

Project *TransitionRemoveCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(track_->parent())->project();
}

void TransitionRemoveCommand::redo_internal()
{
  track_->BeginOperation();

  TimeRange invalidate_range(block_->in(), block_->out());

  if (in_block_) {
    in_block_->set_length_and_media_in(in_block_->length() + block_->in_offset());
  }

  if (out_block_) {
    out_block_->set_length_and_media_out(out_block_->length() + block_->out_offset());
  }

  if (in_block_) {
    NodeParam::DisconnectEdge(in_block_->output(), block_->in_block_input());
  }

  if (out_block_) {
    NodeParam::DisconnectEdge(out_block_->output(), block_->out_block_input());
  }

  track_->RippleRemoveBlock(block_);

  track_->EndOperation();

  track_->InvalidateCache(invalidate_range, track_->block_input(), track_->block_input());
}

void TransitionRemoveCommand::undo_internal()
{
  track_->BeginOperation();

  if (in_block_) {
    track_->InsertBlockBefore(block_, in_block_);
  } else {
    track_->InsertBlockAfter(block_, out_block_);
  }

  if (in_block_) {
    NodeParam::ConnectEdge(in_block_->output(), block_->in_block_input());
  }

  if (out_block_) {
    NodeParam::ConnectEdge(out_block_->output(), block_->out_block_input());
  }

  // These if statements must be separated because in_offset and out_offset report different things
  // if only one block is connected vs two. So we have to connect the blocks first before we have
  // an accurate return value from these offset functions.
  if (in_block_) {
    in_block_->set_length_and_media_in(in_block_->length() - block_->in_offset());
  }

  if (out_block_) {
    out_block_->set_length_and_media_out(out_block_->length() - block_->out_offset());
  }

  track_->EndOperation();

  track_->InvalidateCache(TimeRange(block_->in(), block_->out()), track_->block_input(), track_->block_input());
}

OLIVE_NAMESPACE_EXIT
