/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef TIMELINEUNDOABLE_H
#define TIMELINEUNDOABLE_H

#include "config/config.h"
#include "core.h"
#include "node/block/block.h"
#include "node/block/clip/clip.h"
#include "node/block/gap/gap.h"
#include "node/block/transition/transition.h"
#include "node/math/math/math.h"
#include "node/math/merge/merge.h"
#include "node/graph.h"
#include "node/output/track/track.h"
#include "node/output/track/tracklist.h"
#include "timeline/timelinepoints.h"
#include "undo/undocommand.h"
#include "widget/nodeview/nodeviewundo.h"

namespace olive {

inline bool NodeCanBeRemoved(Node* n)
{
  //return n->edges().isEmpty() && n->GetExclusiveDependencies().isEmpty();
  return n->output_connections().empty();
}

inline UndoCommand* CreateRemoveCommand(Node* n)
{
  return new NodeRemoveWithExclusiveDependenciesAndDisconnect(n);
}

inline UndoCommand* CreateAndRunRemoveCommand(Node* n)
{
  UndoCommand* command = CreateRemoveCommand(n);
  command->redo();
  return command;
}

class BlockResizeCommand : public UndoCommand {
public:
  BlockResizeCommand(Block* block, rational new_length) :
    block_(block),
    new_length_(new_length)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return block_->project();
  }

  virtual void redo() override
  {
    old_length_ = block_->length();
    block_->set_length_and_media_out(new_length_);
  }

  virtual void undo() override
  {
    block_->set_length_and_media_out(old_length_);
  }

private:
  Block* block_;
  rational old_length_;
  rational new_length_;

};

class BlockResizeWithMediaInCommand : public UndoCommand {
public:
  BlockResizeWithMediaInCommand(Block* block, rational new_length) :
    block_(block),
    new_length_(new_length)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return block_->project();
  }

  virtual void redo() override
  {
    old_length_ = block_->length();
    block_->set_length_and_media_in(new_length_);
  }

  virtual void undo() override
  {
    block_->set_length_and_media_in(old_length_);
  }

private:
  Block* block_;
  rational old_length_;
  rational new_length_;
};

/**
 * @brief Performs a trim in the timeline that only affects the block and the block adjacent
 *
 * Changes the length of one block while also changing the length of the block directly adjacent
 * to compensate so that the rest of the track is unaffected.
 *
 * By default, this will only affect the length of gaps. If the adjacent needs to increase its
 * length and is not a gap, a gap will be created and inserted to fill that time. This command can
 * be set to always trim even if the adjacent clip isn't a gap with SetTrimIsARollEdit()
 */
class BlockTrimCommand : public UndoCommand {
public:
  BlockTrimCommand(Track *track, Block* block, rational new_length, Timeline::MovementMode mode) :
    prepped_(false),
    track_(track),
    block_(block),
    new_length_(new_length),
    mode_(mode),
    deleted_adjacent_command_(nullptr),
    trim_is_a_roll_edit_(false)
  {
  }

  virtual ~BlockTrimCommand() override
  {
    delete deleted_adjacent_command_;
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

  /**
   * @brief Set this if the trim should always affect the adjacent clip and not create a gap
   */
  void SetTrimIsARollEdit(bool e)
  {
    trim_is_a_roll_edit_ = e;
  }

  /**
   * @brief Set whether adjacent blocks set to zero length should be removed from the whole graph
   *
   * If an adjacent block's length is set to 0, it's automatically removed from the track. By
   * default it also gets removed from the whole graph. Set this to FALSE to disable that
   * functionality.
   */
  void SetRemoveZeroLengthFromGraph(bool e)
  {
    remove_block_from_graph_ = e;
  }

  virtual void redo() override
  {
    if (!prepped_) {
      prep();
      prepped_ = true;
    }

    if (doing_nothing_) {
      return;
    }

    // Begin an operation since we'll be doing a lot
    track_->BeginOperation();

    // Determine how much time to invalidate
    TimeRange invalidate_range;

    if (mode_ == Timeline::kTrimIn) {
      invalidate_range = TimeRange(block_->in(), block_->in() + trim_diff_);
      block_->set_length_and_media_in(new_length_);
    } else {
      invalidate_range = TimeRange(block_->out(), block_->out() - trim_diff_);
      block_->set_length_and_media_out(new_length_);
    }

    if (needs_adjacent_) {
      if (we_created_adjacent_) {
        // Add adjacent and insert it
        adjacent_->setParent(track_->parent());

        if (mode_ == Timeline::kTrimIn) {
          track_->InsertBlockBefore(adjacent_, block_);
        } else {
          track_->InsertBlockAfter(adjacent_, block_);
        }
      } else if (we_removed_adjacent_) {
        track_->RippleRemoveBlock(adjacent_);

        // It no longer inputs/outputs anything, remove it
        if (remove_block_from_graph_ && NodeCanBeRemoved(adjacent_)) {
          if (!deleted_adjacent_command_) {
            deleted_adjacent_command_ = CreateAndRunRemoveCommand(adjacent_);
          } else {
            deleted_adjacent_command_->redo();
          }
        }
      } else {
        rational adjacent_length = adjacent_->length() + trim_diff_;

        if (mode_ == Timeline::kTrimIn) {
          adjacent_->set_length_and_media_out(adjacent_length);
        } else {
          adjacent_->set_length_and_media_in(adjacent_length);
        }
      }
    }

    track_->EndOperation();

    if (block_->type() == Block::kTransition) {
      // Whole transition needs to be invalidated
      invalidate_range = block_->range();
    }

    track_->Node::InvalidateCache(invalidate_range, Track::kBlockInput);
  }

  virtual void undo() override
  {
    if (doing_nothing_) {
      return;
    }

    track_->BeginOperation();

    // Will be POSITIVE if trimming shorter and NEGATIVE if trimming longer
    if (needs_adjacent_) {
      if (we_created_adjacent_) {
        // Adjacent is ours, just delete it
        track_->RippleRemoveBlock(adjacent_);
        adjacent_->setParent(&memory_manager_);
      } else {
        if (we_removed_adjacent_) {
          if (deleted_adjacent_command_) {
            // We deleted adjacent, restore it now
            deleted_adjacent_command_->undo();
          }

          if (mode_ == Timeline::kTrimIn) {
            track_->InsertBlockBefore(adjacent_, block_);
          } else {
            track_->InsertBlockAfter(adjacent_, block_);
          }
        } else {
          rational adjacent_length = adjacent_->length() - trim_diff_;

          if (mode_ == Timeline::kTrimIn) {
            adjacent_->set_length_and_media_out(adjacent_length);
          } else {
            adjacent_->set_length_and_media_in(adjacent_length);
          }
        }
      }
    }

    TimeRange invalidate_range;

    if (mode_ == Timeline::kTrimIn) {
      block_->set_length_and_media_in(old_length_);

      invalidate_range = TimeRange(block_->in(), block_->in() + trim_diff_);
    } else {
      block_->set_length_and_media_out(old_length_);

      invalidate_range = TimeRange(block_->out(), block_->out() - trim_diff_);
    }

    if (block_->type() == Block::kTransition) {
      // Whole transition needs to be invalidated
      invalidate_range = block_->range();
    }

    track_->EndOperation();

    track_->Node::InvalidateCache(invalidate_range, Track::kBlockInput);
  }

private:
  void prep()
  {
    // Store old length
    old_length_ = block_->length();

    // Determine if the length isn't changing, in which case we set a flag to do nothing
    if ((doing_nothing_ = (old_length_ == new_length_))) {
      return;
    }

    // Will be POSITIVE if trimming shorter and NEGATIVE if trimming longer
    trim_diff_ = old_length_ - new_length_;

    // Retrieve our adjacent block (or nullptr if none)
    if (mode_ == Timeline::kTrimIn) {
      adjacent_ = block_->previous();
    } else {
      adjacent_ = block_->next();
    }

    // Ignore when trimming the out with no adjacent, because the user must have trimmed the end
    // of the last block in the track, so we don't need to do anything elses
    needs_adjacent_ = (mode_ == Timeline::kTrimIn || adjacent_);

    if (needs_adjacent_) {
      // If we're trimming shorter, we need an adjacent, so check if we have a viable one.
      we_created_adjacent_ = (trim_diff_ > 0 && (!adjacent_ || (adjacent_->type() != Block::kGap && !trim_is_a_roll_edit_)));

      if (we_created_adjacent_) {
        // We shortened but don't have a viable adjacent to lengthen, so we create one
        adjacent_ = new GapBlock();
        adjacent_->set_length_and_media_out(trim_diff_);
      } else {
        // Determine if we're removing the adjacent
        rational adjacent_length = adjacent_->length() + trim_diff_;
        we_removed_adjacent_ = adjacent_length.isNull();
      }
    }
  }

  bool prepped_;
  bool doing_nothing_;
  rational trim_diff_;

  Track* track_;
  Block* block_;
  rational old_length_;
  rational new_length_;
  Timeline::MovementMode mode_;

  Block* adjacent_;
  bool needs_adjacent_;
  bool we_created_adjacent_;
  bool we_removed_adjacent_;
  UndoCommand* deleted_adjacent_command_;

  bool trim_is_a_roll_edit_;
  bool remove_block_from_graph_;

  QObject memory_manager_;

};

class BlockSetMediaInCommand : public UndoCommand {
public:
  BlockSetMediaInCommand(Block* block, rational new_media_in) :
    block_(block),
    new_media_in_(new_media_in)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return block_->project();
  }

  virtual void redo() override
  {
    old_media_in_ = block_->media_in();
    block_->set_media_in(new_media_in_);
  }

  virtual void undo() override
  {
    block_->set_media_in(old_media_in_);
  }

private:
  Block* block_;
  rational old_media_in_;
  rational new_media_in_;
};

class TrackRippleRemoveBlockCommand : public UndoCommand {
public:
  TrackRippleRemoveBlockCommand(Track* track, Block* block) :
    track_(track),
    block_(block)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

  virtual void redo() override
  {
    before_ = block_->previous();
    track_->RippleRemoveBlock(block_);
  }

  virtual void undo() override
  {
    track_->InsertBlockAfter(block_, before_);
  }

private:
  Track* track_;

  Block* block_;

  Block* before_;

};

class TrackPrependBlockCommand : public UndoCommand {
public:
  TrackPrependBlockCommand(Track* track, Block* block) :
    track_(track),
    block_(block)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

  virtual void redo() override
  {
    track_->PrependBlock(block_);
  }

  virtual void undo() override
  {
    track_->RippleRemoveBlock(block_);
  }

private:
  Track* track_;
  Block* block_;
};

class TrackInsertBlockAfterCommand : public UndoCommand {
public:
  TrackInsertBlockAfterCommand(Track* track, Block* block, Block* before) :
    track_(track),
    block_(block),
    before_(before)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return block_->project();
  }

  virtual void redo() override
  {
    track_->InsertBlockAfter(block_, before_);
  }

  virtual void undo() override
  {
    track_->RippleRemoveBlock(block_);
  }

private:
  Track* track_;

  Block* block_;

  Block* before_;
};

class BlockSplitCommand : public UndoCommand {
public:
  BlockSplitCommand(Block* block, rational point) :
    block_(block),
    new_block_(nullptr),
    point_(point),
    reconnect_tree_command_(nullptr)
  {
  }

  virtual ~BlockSplitCommand() override
  {
    delete reconnect_tree_command_;
  }

  virtual Project* GetRelevantProject() const override
  {
    return block_->project();
  }

  /**
   * @brief Access the second block created as a result. Only valid after redo().
   */
  Block* new_block()
  {
    return new_block_;
  }

  virtual void redo() override
  {
    old_length_ = block_->length();

    Q_ASSERT(point_ > block_->in() && point_ < block_->out());

    if (!reconnect_tree_command_) {
      reconnect_tree_command_ = new MultiUndoCommand();
      new_block_ = static_cast<Block*>(Node::CopyNodeInGraph(block_, reconnect_tree_command_));
    }

    reconnect_tree_command_->redo();

    // Determine our new lengths
    rational new_length = point_ - block_->in();
    rational new_part_length = block_->out() - point_;

    // Begin an operation
    Track* track = block_->track();
    track->BeginOperation();

    // Set lengths
    block_->set_length_and_media_out(new_length);
    new_block()->set_length_and_media_in(new_part_length);

    // Insert new block
    track->InsertBlockAfter(new_block(), block_);

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

    track->EndOperation();
  }

  virtual void undo() override
  {
    Track* track = block_->track();

    track->BeginOperation();

    if (moved_transition_.IsValid()) {
      Node::DisconnectEdge(new_block(), moved_transition_);
      Node::ConnectEdge(block_, moved_transition_);
    }

    block_->set_length_and_media_out(old_length_);
    track->RippleRemoveBlock(new_block());

    // If we ran a reconnect command, disconnect now
    reconnect_tree_command_->undo();

    track->EndOperation();
  }

private:
  Block* block_;
  Block* new_block_;

  rational old_length_;
  rational point_;

  MultiUndoCommand* reconnect_tree_command_;

  NodeInput moved_transition_;

};

class BlockSplitPreservingLinksCommand : public UndoCommand {
public:
  BlockSplitPreservingLinksCommand(const QVector<Block *> &blocks, const QList<rational>& times) :
    blocks_(blocks),
    times_(times)
  {
  }

  virtual ~BlockSplitPreservingLinksCommand() override
  {
    qDeleteAll(commands_);
  }

  virtual Project* GetRelevantProject() const override
  {
    return blocks_.first()->project();
  }

  virtual void redo() override
  {
    if (commands_.isEmpty()) {
      QVector< QVector<Block*> > split_blocks(times_.size());

      for (int i=0;i<times_.size();i++) {
        const rational& time = times_.at(i);

        // FIXME: I realize this isn't going to work if the times aren't ordered. I'm lazy so rather
        //        than writing in a sorting algorithm here, I'll just put an assert as a reminder
        //        if this ever becomes an issue.
        Q_ASSERT(i == 0 || time > times_.at(i-1));

        QVector<Block*> splits(blocks_.size());
        commands_.resize(blocks_.size());

        for (int j=0;j<blocks_.size();j++) {
          Block* b = blocks_.at(j);

          if (b->in() < time && b->out() > time) {
            BlockSplitCommand* split_command = new BlockSplitCommand(b, time);
            split_command->redo();
            splits.replace(j, split_command->new_block());
            commands_.replace(j, split_command);
          } else {
            splits.replace(j, nullptr);
          }
        }

        split_blocks.replace(i, splits);
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

            foreach (const QVector<Block*>& split_list, split_blocks) {
              NodeLinkCommand* blc = new NodeLinkCommand(split_list.at(i), split_list.at(j), true);
              blc->redo();
              commands_.append(blc);
            }
          }
        }
      }
    } else {
      for (int i=0; i<commands_.size(); i++) {
        commands_.at(i)->redo();
      }
    }
  }

  virtual void undo() override
  {
    for (int i=commands_.size()-1; i>=0; i--) {
      commands_.at(i)->undo();
    }
  }

private:
  QVector<Block *> blocks_;

  QList<rational> times_;

  QVector<UndoCommand*> commands_;

};

class TrackSplitAtTimeCommand : public UndoCommand {
public:
  TrackSplitAtTimeCommand(Track* track, rational point) :
    prepped_(false),
    track_(track),
    point_(point),
    command_(nullptr)
  {
  }

  virtual ~TrackSplitAtTimeCommand() override
  {
    delete command_;
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

  virtual void redo() override
  {
    if (!prepped_) {
      // Find Block that contains this time
      Block* b = track_->BlockContainingTime(point_);

      if (b) {
        command_ = new BlockSplitCommand(b, point_);
      }

      prepped_ = true;
    }

    if (command_) {
      command_->redo();
    }
  }

  virtual void undo() override
  {
    if (command_) {
      command_->undo();
    }
  }

private:
  bool prepped_;

  Track* track_;

  rational point_;

  UndoCommand* command_;

};

/**
 * @brief Clears the area between in and out
 *
 * The area between `in` and `out` is guaranteed to be freed. BLocks are trimmed and removed to free this space.
 * By default, nothing takes this area meaning all subsequent clips are pushed backward, however you can specify
 * a block to insert at the `in` point. No checking is done to ensure `insert` is the same length as `in` to `out`.
 */
class TrackRippleRemoveAreaCommand : public UndoCommand {
public:
  TrackRippleRemoveAreaCommand(Track* track, const TimeRange& range) :
    prepped_(false),
    track_(track),
    range_(range),
    splice_split_command_(nullptr)
  {
    trim_out_.block = nullptr;
    trim_in_.block = nullptr;
  }

  virtual ~TrackRippleRemoveAreaCommand() override
  {
    delete splice_split_command_;
    qDeleteAll(remove_block_commands_);
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

  /**
   * @brief Block to insert after if you want to insert something between this ripple
   */
  Block* GetInsertionIndex() const
  {
    return insert_previous_;
  }

  Block* GetSplicedBlock() const
  {
    if (splice_split_command_) {
      return splice_split_command_->new_block();
    }

    return nullptr;
  }

  virtual void redo() override
  {
    if (!prepped_) {
      prep();
      prepped_ = true;
    }

    track_->BeginOperation();

    if (splice_split_command_) {
      // We're just splicing
      splice_split_command_->redo();

      // Trim the in of the split
      Block* split = splice_split_command_->new_block();
      split->set_length_and_media_in(split->length() - (range_.out() - split->in()));
    } else {
      if (trim_out_.block) {
        trim_out_.block->set_length_and_media_out(trim_out_.new_length);
      }

      if (trim_in_.block) {
        trim_in_.block->set_length_and_media_in(trim_in_.new_length);
      }

      // Perform removals
      if (!removals_.isEmpty()) {
        foreach (auto op, removals_) {
          // Ripple remove them all first
          track_->RippleRemoveBlock(op.block);
        }

        // Create undo commands for node removals where possible
        if (remove_block_commands_.isEmpty()) {
          foreach (auto op, removals_) {
            if (NodeCanBeRemoved(op.block)) {
              remove_block_commands_.append(CreateRemoveCommand(op.block));
            }
          }
        }

        foreach (UndoCommand* c, remove_block_commands_) {
          c->redo();
        }
      }
    }

    track_->EndOperation();

    track_->Node::InvalidateCache(TimeRange(range_.in(), RATIONAL_MAX), Track::kBlockInput);
  }

  virtual void undo() override
  {
    // Begin operations
    track_->BeginOperation();

    if (splice_split_command_) {
      splice_split_command_->undo();
    } else {
      if (trim_out_.block) {
        trim_out_.block->set_length_and_media_out(trim_out_.old_length);
      }

      if (trim_in_.block) {
        trim_in_.block->set_length_and_media_in(trim_in_.old_length);
      }

      // Un-remove any blocks
      for (int i=remove_block_commands_.size()-1; i>=0; i--) {
        remove_block_commands_.at(i)->undo();
      }

      foreach (auto op, removals_) {
        track_->InsertBlockAfter(op.block, op.before);
      }
    }

    // End operations and invalidate
    track_->EndOperation();

    track_->Node::InvalidateCache(TimeRange(range_.in(), RATIONAL_MAX), Track::kBlockInput);
  }

private:
  bool prepped_;

  void prep()
  {
    // Determine precisely what will be happening to these tracks
    Block* first_block = track_->NearestBlockBeforeOrAt(range_.in());

    if (!first_block) {
      // No blocks at this time, nothing to be done on this track
      return;
    }

    // Determine if this first block is getting trimmed or removed
    bool first_block_is_out_trimmed = first_block->in() < range_.in();
    bool first_block_is_in_trimmed = first_block->out() > range_.out();

    // Set's the block that any insert command should insert AFTER. If the first block is not
    // getting out-trimmed, that means first block is either getting removed or in-trimmed, which
    // means any insert should happen before it
    insert_previous_ = first_block_is_out_trimmed ? first_block : first_block->previous();

    // If it's getting trimmed, determine if it's actually getting spliced
    if (first_block_is_out_trimmed && first_block_is_in_trimmed) {
      // This block is getting spliced, so we'll handle that later
      splice_split_command_ = new BlockSplitCommand(first_block, range_.in());
    } else {
      // It's just getting trimmed or removed, so we'll append that operation
      if (first_block_is_out_trimmed) {
        trim_out_ = {first_block,
                     first_block->length(),
                     first_block->length() - (first_block->out() - range_.in())};
      } else if (first_block_is_in_trimmed) {
        // Block is getting in trimmed
        trim_in_ = {first_block,
                   first_block->length(),
                   first_block->length() - (range_.out() - first_block->in())};
      } else {
        // We know for sure this block is within the range so it will be removed
        removals_.append(RemoveOperation({first_block, first_block->previous()}));
      }

      // If the first block is getting in trimmed, we're already at the end of our range
      if (!first_block_is_in_trimmed) {
        // Loop through the rest of the blocks and determine what to do with those
        for (Block* next=first_block->next(); next; next=next->next()) {
          bool trimming = (next->out() > range_.out());

          if (trimming) {
            trim_in_ = {next,
                        next->length(),
                        next->length() - (range_.out() - next->in())};
            break;
          } else {
            removals_.append(RemoveOperation({next, next->previous()}));

            if (next->out() == range_.out()) {
              break;
            }
          }
        }
      }
    }
  }

  struct TrimOperation {
    Block* block;
    rational old_length;
    rational new_length;
  };

  struct RemoveOperation {
    Block* block;
    Block* before;
  };

  Track* track_;
  TimeRange range_;

  TrimOperation trim_out_;
  QVector<RemoveOperation> removals_;
  TrimOperation trim_in_;
  Block* insert_previous_;

  BlockSplitCommand* splice_split_command_;
  QVector<UndoCommand*> remove_block_commands_;

};

class TrackListRippleRemoveAreaCommand : public UndoCommand {
public:
  TrackListRippleRemoveAreaCommand(TrackList* list, rational in, rational out) :
    list_(list),
    in_(in),
    out_(out)
  {
  }

  virtual ~TrackListRippleRemoveAreaCommand() override
  {
    qDeleteAll(commands_);
  }

  virtual Project* GetRelevantProject() const override
  {
    return list_->parent()->project();
  }

  virtual void redo() override
  {
    // Code that's only run on the first redo
    if (commands_.isEmpty()) {
      all_tracks_unlocked_ = true;

      foreach (Track* track, list_->GetTracks()) {
        if (track->IsLocked()) {
          all_tracks_unlocked_ = false;
          continue;
        }

        TrackRippleRemoveAreaCommand* c = new TrackRippleRemoveAreaCommand(track, TimeRange(in_, out_));
        commands_.append(c);
        working_tracks_.append(track);
      }
    }

    if (all_tracks_unlocked_) {
      // We can optimize here by simply shifting the whole cache forward instead of re-caching
      // everything following this time
      if (list_->type() == Track::kVideo) {
        list_->parent()->ShiftVideoCache(out_, in_);
      } else if (list_->type() == Track::kAudio) {
        list_->parent()->ShiftAudioCache(out_, in_);
      }

      foreach (Track* track, working_tracks_) {
        track->BeginOperation();
      }
    }

    foreach (TrackRippleRemoveAreaCommand* c, commands_) {
      c->redo();
    }

    if (all_tracks_unlocked_) {
      foreach (Track* track, working_tracks_) {
        track->EndOperation();
      }
    }
  }

  virtual void undo() override
  {
    if (all_tracks_unlocked_) {
      // We can optimize here by simply shifting the whole cache forward instead of re-caching
      // everything following this time
      if (list_->type() == Track::kVideo) {
        list_->parent()->ShiftVideoCache(in_, out_);
      } else if (list_->type() == Track::kAudio) {
        list_->parent()->ShiftAudioCache(in_, out_);
      }

      foreach (Track* track, working_tracks_) {
        track->BeginOperation();
      }
    }

    foreach (TrackRippleRemoveAreaCommand* c, commands_) {
      c->undo();
    }

    if (all_tracks_unlocked_) {
      foreach (Track* track, working_tracks_) {
        track->EndOperation();
      }
    }
  }

private:
  TrackList* list_;

  QList<Track*> working_tracks_;

  rational in_;

  rational out_;

  bool all_tracks_unlocked_;

  QVector<TrackRippleRemoveAreaCommand*> commands_;

};

class TimelineRippleRemoveAreaCommand : public MultiUndoCommand {
public:
  TimelineRippleRemoveAreaCommand(Sequence* timeline, rational in, rational out) :
    timeline_(timeline)
  {
    for (int i=0; i<Track::kCount; i++) {
      add_child(new TrackListRippleRemoveAreaCommand(timeline->track_list(static_cast<Track::Type>(i)),
                                                     in,
                                                     out));
    }
  }

  virtual Project* GetRelevantProject() const override
  {
    return timeline_->project();
  }

private:
  Sequence* timeline_;

};

class TrackListRippleToolCommand : public UndoCommand {
public:
  struct RippleInfo {
    Block* block;
    bool append_gap;
  };

  TrackListRippleToolCommand(TrackList* track_list,
                             const QHash<Track*, RippleInfo>& info,
                             const rational& ripple_movement,
                             const Timeline::MovementMode& movement_mode) :
    track_list_(track_list),
    info_(info),
    ripple_movement_(ripple_movement),
    movement_mode_(movement_mode)
  {
    all_tracks_unlocked_ = (info_.size() == track_list_->GetTrackCount());
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_list_->parent()->project();
  }

  virtual void redo() override
  {
    ripple(true);
  }

  virtual void undo() override
  {
    ripple(false);
  }

private:
  void ripple(bool redo)
  {
    if (info_.isEmpty()) {
      return;
    }

    // The following variables are used to determine how much of the cache to invalidate

    // If we can shift, we will shift from the latest out before the ripple to the latest out after,
    // since those sections will be unchanged by this ripple
    rational pre_latest_out = RATIONAL_MIN;
    rational post_latest_out = RATIONAL_MIN;

    // Make timeline changes
    for (auto it=info_.cbegin(); it!=info_.cend(); it++) {
      Track* track = it.key();
      const RippleInfo& info = it.value();
      WorkingData working_data = working_data_.value(track);
      Block* b = info.block;

      // Generate block length
      rational new_block_length;
      rational operation_movement = ripple_movement_;

      if (movement_mode_ == Timeline::kTrimIn) {
        operation_movement = -operation_movement;
      }

      if (!redo) {
        operation_movement = -operation_movement;
      }

      if (b) {
        new_block_length = b->length() + operation_movement;
      }

      rational earliest_point_of_change;
      rational pre_shift;
      rational post_shift;

      // Begin operation so we can invalidate better later
      track->BeginOperation();

      if (info.append_gap) {

        // Rather than rippling the referenced block, we'll insert a gap and ripple with that
        GapBlock* gap = working_data.created_gap;

        if (redo) {
          if (!gap) {
            gap = new GapBlock();
            gap->set_length_and_media_out(qAbs(ripple_movement_));
            working_data.created_gap = gap;
          }

          gap->setParent(track->parent());
          track->InsertBlockBefore(gap, b);

          // As an insertion, we will shift from the gap's in to the gap's out
          pre_shift = gap->in();
          post_shift = gap->out();
          earliest_point_of_change = gap->in();
        } else {
          // As a removal, we will shift from the gap's out to the gap's in
          pre_shift = gap->out();
          post_shift = gap->in();

          track->RippleRemoveBlock(gap);
          gap->setParent(&memory_manager_);
        }

      } else if ((redo && new_block_length.isNull()) || (!redo && !b->track())) {

        // The ripple is the length of this block. We assume that for this to happen, it must have
        // been a gap that we will now remove.

        if (redo) {
          // The earliest point changes will happen is at the start of this block
          earliest_point_of_change = b->in();

          // As a removal, we will be shifting from the out point to the in point
          pre_shift = b->out();
          post_shift = b->in();

          // Remove gap from track and from graph
          working_data.removed_gap_after = b->previous();
          track->RippleRemoveBlock(b);
          b->setParent(&memory_manager_);
        } else {
          // Restore gap to graph and track
          b->setParent(track->parent());
          track->InsertBlockAfter(b, working_data.removed_gap_after);

          // The earliest point changes will happen is at the start of this block
          earliest_point_of_change = b->in();

          // As an insert, we will be shifting from the block's in point to its out point
          pre_shift = b->in();
          post_shift = b->out();
        }

      } else {

        // Store old length
        working_data.old_length = b->length();

        if (movement_mode_ == Timeline::kTrimIn) {
          // The earliest point changes will occur is in point of this bloc
          earliest_point_of_change = b->in();

          // Undo the trim in inversion we do above, this will still be inverted accurately for
          // undoing where appropriate
          rational inverted = -operation_movement;
          if (inverted > 0) {
            pre_shift = b->in() + inverted;
            post_shift = b->in();
          } else {
            pre_shift = b->in();
            post_shift = b->in() - inverted;
          }

          // Update length
          b->set_length_and_media_in(new_block_length);
        } else {
          // The earliest point changes will occur is the out point if trimming out or the in point
          // if trimming in
          earliest_point_of_change = b->out();

          // The latest out before the ripple is this block's current out point
          pre_shift = b->out();

          // Update length
          b->set_length_and_media_out(new_block_length);

          // The latest out after the ripple is this block's out point after the length change
          post_shift = b->out();
        }

      }

      track->EndOperation();

      working_data_.insert(it.key(), working_data);

      pre_latest_out = qMax(pre_latest_out, pre_shift);
      post_latest_out = qMax(post_latest_out, post_shift);

      if (!all_tracks_unlocked_) {
        // If we're not shifting, the whole track must get invalidated
        track->Node::InvalidateCache(TimeRange(earliest_point_of_change, RATIONAL_MAX), Track::kBlockInput);
      }
    }

    if (all_tracks_unlocked_) {
      // We rippled all the tracks, so we can shift the whole cache
      if (track_list_->type() == Track::kVideo) {
        track_list_->parent()->ShiftVideoCache(pre_latest_out, post_latest_out);
      } else if (track_list_->type() == Track::kAudio) {
        track_list_->parent()->ShiftAudioCache(pre_latest_out, post_latest_out);
      }
    }
  }

  TrackList* track_list_;

  QHash<Track*, RippleInfo> info_;
  rational ripple_movement_;
  Timeline::MovementMode movement_mode_;

  struct WorkingData {
    GapBlock* created_gap = nullptr;
    Block* removed_gap_after;
    rational old_length;
  };

  QHash<Track*, WorkingData> working_data_;

  QObject memory_manager_;

  bool all_tracks_unlocked_;

};

class TimelineAddTrackCommand : public UndoCommand {
public:
  TimelineAddTrackCommand(TrackList *timeline) :
    timeline_(timeline),
    position_command_(nullptr)
  {
    track_ = new Track();
    track_->setParent(&memory_manager_);

    if (timeline->GetTrackCount() > 0 && Config::Current()[QStringLiteral("AutoMergeTracks")].toBool()) {
      if (timeline_->type() == Track::kVideo) {
        merge_ = new MergeNode();
        base_ = NodeInput(merge_, MergeNode::kBaseIn);
        blend_ = NodeInput(merge_, MergeNode::kBlendIn);
      } else {
        merge_ = new MathNode();
        base_ = NodeInput(merge_, MathNode::kParamAIn);
        blend_ = NodeInput(merge_, MathNode::kParamBIn);
      }
      merge_->setParent(&memory_manager_);
    } else {
      merge_ = nullptr;
    }
  }

  virtual ~TimelineAddTrackCommand() override
  {
    delete position_command_;
  }

  Track* track() const
  {
    return track_;
  }

  virtual Project* GetRelevantProject() const override
  {
    return timeline_->parent()->project();
  }

  virtual void redo() override
  {
    // Add track
    track_->setParent(timeline_->GetParentGraph());
    timeline_->ArrayAppend();
    int track_total_index = timeline_->parent()->GetTracks().size();
    if (!position_command_) {
      position_command_ = new NodeSetPositionAsChildCommand(track_, timeline_->parent(), track_total_index, track_total_index + 1, true);
    }
    position_command_->redo();
    Node::ConnectEdge(track_, timeline_->track_input(timeline_->ArraySize() - 1));

    // Add merge if applicable
    if (merge_) {
      merge_->setParent(timeline_->GetParentGraph());

      Track* last_track = timeline_->GetTrackAt(timeline_->GetTrackCount()-2);

      // Whatever this track used to be connected to, connect the merge instead
      const Node::OutputConnections edges = last_track->output_connections();
      for (const Node::OutputConnection& ic : edges) {
        const NodeInput& i = ic.second;

        // Ignore the track input, but funnel everything else through our merge
        if (i.node() != timeline_->parent() || i.input() != timeline_->track_input()) {
          Node::DisconnectEdge(last_track, i);
          Node::ConnectEdge(merge_, i);
        }
      }

      // Connect this as the "blend" track
      Node::ConnectEdge(track_, blend_);
      Node::ConnectEdge(last_track, base_);
    } else if (timeline_->GetTrackCount() == 1) {
      // If this was the first track we added,
      QString relevant_input;

      if (timeline_->type() == Track::kVideo) {
        relevant_input = Sequence::kTextureInput;
      } else {
        relevant_input = Sequence::kSamplesInput;
      }

      if (!timeline_->parent()->IsInputConnected(relevant_input)) {
        direct_ = NodeInput(timeline_->parent(), relevant_input);

        Node::ConnectEdge(track_, direct_);
      } else {
        direct_ = NodeInput();
      }
    }
  }

  virtual void undo() override
  {
    // Remove merge if applicable
    if (merge_) {
      // Assume whatever this merge is connected to USED to be connected to the last track
      Track* last_track = timeline_->GetTrackAt(timeline_->GetTrackCount()-2);

      Node::DisconnectEdge(track_, blend_);
      Node::DisconnectEdge(last_track, base_);

      // Make copy of edges since the node's internal array will change as we disconnect things
      const Node::OutputConnections edges = merge_->output_connections();
      for (const Node::OutputConnection& ic : edges) {
        const NodeInput& i = ic.second;

        Node::DisconnectEdge(merge_, i);
        Node::ConnectEdge(last_track, i);
      }

      merge_->setParent(&memory_manager_);
    } else if (direct_.IsValid()) {
      Node::DisconnectEdge(track_, direct_);
    }

    // Remove track
    Node::DisconnectEdge(track_, timeline_->track_input(timeline_->ArraySize() - 1));
    position_command_->undo();
    timeline_->ArrayRemoveLast();
    track_->setParent(&memory_manager_);
  }

private:
  TrackList* timeline_;

  Track* track_;
  Node* merge_;
  NodeInput base_;
  NodeInput blend_;

  NodeInput direct_;

  NodeSetPositionAsChildCommand* position_command_;

  QObject memory_manager_;

};

/**
 * @brief Destructively places `block` at the in point `start`
 *
 * The Block is guaranteed to be placed at the starting point specified. If there are Blocks in this area, they are
 * either trimmed or removed to make space for this Block. Additionally, if the Block is placed beyond the end of
 * the Sequence, a GapBlock is inserted to compensate.
 */
class TrackPlaceBlockCommand : public UndoCommand {
public:
  TrackPlaceBlockCommand(TrackList *timeline, int track, Block* block, rational in) :
    timeline_(timeline),
    track_index_(track),
    in_(in),
    gap_(nullptr),
    insert_(block),
    ripple_remove_command_(nullptr)
  {
  }

  virtual ~TrackPlaceBlockCommand() override
  {
    delete ripple_remove_command_;
    qDeleteAll(add_track_commands_);
  }

  virtual Project* GetRelevantProject() const override
  {
    return timeline_->parent()->project();
  }

  virtual void redo() override
  {
    // Determine if we need to add tracks
    if (track_index_ >= timeline_->GetTracks().size()) {
      if (add_track_commands_.isEmpty()) {
        // First redo, create tracks now
        add_track_commands_.resize(track_index_ - timeline_->GetTracks().size() + 1);

        for (int i=0; i<add_track_commands_.size(); i++) {
          add_track_commands_[i] = new TimelineAddTrackCommand(timeline_);
        }
      }

      for (int i=0; i<add_track_commands_.size(); i++) {
        add_track_commands_.at(i)->redo();
      }
    }

    Track* track = timeline_->GetTrackAt(track_index_);

    bool append = (in_ >= track->track_length());

    // Check if the placement location is past the end of the timeline
    if (append) {
      if (in_ > track->track_length()) {
        // If so, insert a gap here
        if (!gap_) {
          gap_ = new GapBlock();
          gap_->set_length_and_media_out(in_ - track->track_length());
        }
        gap_->setParent(track->parent());
        track->AppendBlock(gap_);
      }

      track->AppendBlock(insert_);
    } else {
      // Place the Block at this point
      if (!ripple_remove_command_) {
        ripple_remove_command_ = new TrackRippleRemoveAreaCommand(track,
                                                                  TimeRange(in_, in_ + insert_->length()));
      }

      ripple_remove_command_->redo();
      track->InsertBlockAfter(insert_, ripple_remove_command_->GetInsertionIndex());
    }
  }

  virtual void undo() override
  {
    Track* t = timeline_->GetTrackAt(track_index_);

    // Firstly, remove our insert
    t->RippleRemoveBlock(insert_);

    if (ripple_remove_command_) {
      // If we ripple removed, just undo that
      ripple_remove_command_->undo();
    } else if (gap_) {
      t->RippleRemoveBlock(gap_);
      gap_->setParent(&memory_manager_);
    }

    // Remove tracks if we added them
    for (int i=add_track_commands_.size()-1; i>=0; i--) {
      add_track_commands_.at(i)->undo();
    }
  }

private:
  TrackList* timeline_;
  int track_index_;
  rational in_;
  GapBlock* gap_;
  Block* insert_;
  QVector<TimelineAddTrackCommand*> add_track_commands_;
  QObject memory_manager_;
  TrackRippleRemoveAreaCommand* ripple_remove_command_;

};

/**
 * @brief Replaces Block `old` with Block `replace`
 *
 * Both blocks must have equal lengths.
 */
class TrackReplaceBlockCommand : public UndoCommand {
public:
  TrackReplaceBlockCommand(Track* track, Block* old, Block* replace) :
    track_(track),
    old_(old),
    replace_(replace)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

  virtual void redo() override
  {
    track_->ReplaceBlock(old_, replace_);
  }

  virtual void undo() override
  {
    track_->ReplaceBlock(replace_, old_);
  }

private:
  Track* track_;
  Block* old_;
  Block* replace_;

};

class TrackReplaceBlockWithGapCommand : public UndoCommand {
public:
  TrackReplaceBlockWithGapCommand(Track* track, Block* block) :
    track_(track),
    block_(block),
    existing_gap_(nullptr),
    existing_merged_gap_(nullptr),
    our_gap_(nullptr)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return block_->project();
  }

  virtual void redo() override
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
        existing_merged_gap_->setParent(&memory_manager_);
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
        if (!our_gap_) {
          our_gap_ = new GapBlock();
          our_gap_->set_length_and_media_out(new_gap_length);
        }

        our_gap_->setParent(track_->parent());
        track_->ReplaceBlock(block_, our_gap_);
      }

    } else {
      // Block is at the end of the track, simply remove it

      // Determine if it's proceeded by a gap, and remove that gap if so
      Block* preceding = block_->previous();
      if (preceding && preceding->type() == Block::kGap) {
        track_->RippleRemoveBlock(preceding);
        preceding->setParent(&memory_manager_);

        existing_merged_gap_ = static_cast<GapBlock*>(preceding);
      }

      // Remove block in question
      track_->RippleRemoveBlock(block_);
    }

    track_->EndOperation();

    track_->Node::InvalidateCache(invalidate_range, Track::kBlockInput);
  }

  virtual void undo() override
  {
    track_->BeginOperation();

    if (our_gap_) {

      // We made this gap, simply swap our gap back
      track_->ReplaceBlock(our_gap_, block_);
      our_gap_->setParent(&memory_manager_);

    } else if (existing_gap_) {

      // If we're here, assume that we extended an existing gap
      rational original_gap_length = existing_gap_->length() - block_->length();

      // If we merged two gaps together, restore the second one now
      if (existing_merged_gap_) {
        original_gap_length -= existing_merged_gap_->length();
        existing_merged_gap_->setParent(track_->parent());
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
        existing_merged_gap_->setParent(track_->parent());
        track_->AppendBlock(existing_merged_gap_);
        existing_merged_gap_ = nullptr;
      }

      // Restore block
      track_->AppendBlock(block_);

    }

    track_->EndOperation();

    track_->Node::InvalidateCache(TimeRange(block_->in(), block_->out()), Track::kBlockInput);
  }

private:
  Track* track_;
  Block* block_;

  GapBlock* existing_gap_;
  GapBlock* existing_merged_gap_;
  bool existing_gap_precedes_;
  GapBlock* our_gap_;

  QObject memory_manager_;

};

class TimelineRippleDeleteGapsAtRegionsCommand : public UndoCommand {
public:
  TimelineRippleDeleteGapsAtRegionsCommand(Sequence* vo, const TimeRangeList& regions) :
    timeline_(vo),
    regions_(regions)
  {
  }

  virtual ~TimelineRippleDeleteGapsAtRegionsCommand() override
  {
    qDeleteAll(commands_);
  }

  virtual Project* GetRelevantProject() const override
  {
    return timeline_->project();
  }

  virtual void redo() override
  {
    if (commands_.isEmpty()) {
      foreach (const TimeRange& range, regions_) {
        rational max_ripple_length = range.length();

        QVector<Block*> blocks_around_range;

        foreach (Track* track, timeline_->GetTracks()) {
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
              commands_.append(new TrackRippleRemoveBlockCommand(resize->track(), resize));
            } else {
              // Resize block
              commands_.append(new BlockResizeCommand(resize, resize->length() - max_ripple_length));
            }
          }
        }
      }
    }

    foreach (UndoCommand* c, commands_) {
      c->redo();
    }
  }

  virtual void undo() override
  {
    for (int i=commands_.size()-1;i>=0;i--) {
      commands_.at(i)->undo();
    }
  }

private:
  Sequence* timeline_;
  TimeRangeList regions_;

  QVector<UndoCommand*> commands_;

};

class WorkareaSetEnabledCommand : public UndoCommand {
public:
  WorkareaSetEnabledCommand(Project *project, TimelinePoints* points, bool enabled) :
    project_(project),
    points_(points),
    old_enabled_(points_->workarea()->enabled()),
    new_enabled_(enabled)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return project_;
  }

  virtual void redo() override
  {
    points_->workarea()->set_enabled(new_enabled_);
  }

  virtual void undo() override
  {
    points_->workarea()->set_enabled(old_enabled_);
  }

private:
  Project* project_;

  TimelinePoints* points_;

  bool old_enabled_;

  bool new_enabled_;

};

class WorkareaSetRangeCommand : public UndoCommand {
public:
  WorkareaSetRangeCommand(Project *project, TimelinePoints* points, const TimeRange& range) :
    project_(project),
    points_(points),
    old_range_(points_->workarea()->range()),
    new_range_(range)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return project_;
  }

  virtual void redo() override
  {
    points_->workarea()->set_range(new_range_);
  }

  virtual void undo() override
  {
    points_->workarea()->set_range(old_range_);
  }

private:
  Project* project_;

  TimelinePoints* points_;

  TimeRange old_range_;

  TimeRange new_range_;

};

class BlockEnableDisableCommand : public UndoCommand {
public:
  BlockEnableDisableCommand(Block* block, bool enabled) :
    block_(block),
    old_enabled_(block_->is_enabled()),
    new_enabled_(enabled)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return block_->project();
  }

  virtual void redo() override
  {
    block_->set_enabled(new_enabled_);
  }

  virtual void undo() override
  {
    block_->set_enabled(old_enabled_);
  }

private:
  Block* block_;

  bool old_enabled_;

  bool new_enabled_;

};

class TrackSlideCommand : public UndoCommand {
public:
  TrackSlideCommand(Track* track, const QList<Block*>& moving_blocks, Block* in_adjacent, Block* out_adjacent, const rational& movement) :
    prepped_(false),
    track_(track),
    blocks_(moving_blocks),
    movement_(movement),
    in_adjacent_(in_adjacent),
    in_adjacent_remove_command_(nullptr),
    out_adjacent_(out_adjacent),
    out_adjacent_remove_command_(nullptr)
  {
    Q_ASSERT(!movement_.isNull());
  }

  virtual ~TrackSlideCommand() override
  {
    delete in_adjacent_remove_command_;
    delete out_adjacent_remove_command_;
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

  virtual void redo() override
  {
    if (!prepped_) {
      prep();
      prepped_ = true;
    }

    // Make sure all movement blocks' old positions are invalidated
    TimeRange invalidate_range(blocks_.first()->in(), blocks_.last()->out());

    track_->BeginOperation();

    // We will always have an in adjacent if there was a valid slide
    if (we_created_in_adjacent_) {
      // We created in adjacent, so all we have to do is insert it
      in_adjacent_->setParent(track_->parent());
      track_->InsertBlockBefore(in_adjacent_, blocks_.first());
    } else if (-movement_ == in_adjacent_->length()) {
      // Movement will remove in adjacent
      track_->RippleRemoveBlock(in_adjacent_);

      if (NodeCanBeRemoved(in_adjacent_)) {
        if (!in_adjacent_remove_command_) {
          in_adjacent_remove_command_ = CreateRemoveCommand(in_adjacent_);
        }

        in_adjacent_remove_command_->redo();
      }
    } else {
      // Simply resize adjacent
      in_adjacent_->set_length_and_media_out(in_adjacent_->length() + movement_);
    }

    // We may not have an out adjacent if the slide was at the end of the track
    if (out_adjacent_) {
      if (we_created_out_adjacent_) {
        // We created out adjacent, so we just have to insert it
        out_adjacent_->setParent(track_->parent());
        track_->InsertBlockAfter(out_adjacent_, blocks_.last());
      } else if (movement_ == out_adjacent_->length()) {
        // Movement will remove out adjacent
        track_->RippleRemoveBlock(out_adjacent_);

        if (NodeCanBeRemoved(out_adjacent_)) {
          if (!out_adjacent_remove_command_) {
            out_adjacent_remove_command_ = CreateRemoveCommand(out_adjacent_);
          }

          out_adjacent_remove_command_->redo();
        }
      } else {
        // Simply resize adjacent
        out_adjacent_->set_length_and_media_in(out_adjacent_->length() - movement_);
      }
    }

    track_->EndOperation();

    // Make sure all movement blocks' new positions are invalidated
    invalidate_range.set_range(qMin(invalidate_range.in(), blocks_.first()->in()),
                               qMax(invalidate_range.out(), blocks_.last()->out()));

    track_->Node::InvalidateCache(invalidate_range, Track::kBlockInput);
  }

  virtual void undo() override
  {
    // Make sure all movement blocks' old positions are invalidated
    TimeRange invalidate_range(blocks_.first()->in(), blocks_.last()->out());

    track_->BeginOperation();

    if (we_created_in_adjacent_) {
      // We created this, so we can remove it now
      track_->RippleRemoveBlock(in_adjacent_);
      in_adjacent_->setParent(&memory_manager_);
    } else if (in_adjacent_remove_command_) {
      // We removed this, so we can restore it now
      in_adjacent_remove_command_->undo();
    } else {
      // Simply resize adjacent
      in_adjacent_->set_length_and_media_out(in_adjacent_->length() - movement_);
    }

    if (out_adjacent_) {
      if (we_created_out_adjacent_) {
        // We created this, so we can remove it now
        track_->RippleRemoveBlock(out_adjacent_);
        out_adjacent_->setParent(&memory_manager_);
      } else if (out_adjacent_remove_command_) {
        out_adjacent_remove_command_->undo();
      } else {
        out_adjacent_->set_length_and_media_in(out_adjacent_->length() + movement_);
      }
    }

    track_->EndOperation();

    // Make sure all movement blocks' new positions are invalidated
    invalidate_range.set_range(qMin(invalidate_range.in(), blocks_.first()->in()),
                               qMax(invalidate_range.out(), blocks_.last()->out()));

    track_->Node::InvalidateCache(invalidate_range, Track::kBlockInput);
  }

private:
  bool prepped_;

  void prep()
  {
    if (!in_adjacent_) {
      in_adjacent_ = new GapBlock();
      in_adjacent_->set_length_and_media_out(movement_);
      in_adjacent_->setParent(&memory_manager_);
      we_created_in_adjacent_ = true;
    } else {
      we_created_in_adjacent_ = false;
    }

    if (!out_adjacent_) {
      if (blocks_.last()->next()) {
        out_adjacent_ = new GapBlock();
        out_adjacent_->set_length_and_media_out(-movement_);
        out_adjacent_->setParent(&memory_manager_);
        we_created_out_adjacent_ = true;
      } else {
        we_created_out_adjacent_ = false;
      }
    }
  }

  Track* track_;
  QList<Block*> blocks_;
  rational movement_;

  bool we_created_in_adjacent_;
  Block* in_adjacent_;
  UndoCommand* in_adjacent_remove_command_;
  bool we_created_out_adjacent_;
  Block* out_adjacent_;
  UndoCommand* out_adjacent_remove_command_;

  QObject memory_manager_;

};

class TrackListInsertGaps : public UndoCommand {
public:
  TrackListInsertGaps(TrackList* track_list, const rational& point, const rational& length) :
    prepped_(false),
    track_list_(track_list),
    point_(point),
    length_(length),
    split_command_(nullptr)
  {
  }

  virtual ~TrackListInsertGaps() override
  {
    delete split_command_;
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_list_->parent()->project();
  }

  virtual void redo() override
  {
    if (!prepped_) {
      prep();
      prepped_ = true;
    }

    if (all_tracks_unlocked_) {
      // Optimize by shifting over since we have a constant amount of time being inserted
      if (track_list_->type() == Track::kVideo) {
        track_list_->parent()->ShiftVideoCache(point_, point_ + length_);
      } else if (track_list_->type() == Track::kAudio) {
        track_list_->parent()->ShiftAudioCache(point_, point_ + length_);
      }
    }

    foreach (Track* track, working_tracks_) {
      track->BeginOperation();
    }

    foreach (Block* gap, gaps_to_extend_) {
      gap->set_length_and_media_out(gap->length() + length_);
    }

    if (split_command_) {
      split_command_->redo();
    }

    foreach (auto add_gap, gaps_added_) {
      add_gap.gap->setParent(add_gap.before->parent());
      add_gap.before->track()->InsertBlockAfter(add_gap.gap, add_gap.before);
    }

    foreach (Track* track, working_tracks_) {
      track->EndOperation();
    }

    if (!all_tracks_unlocked_) {
      foreach (Track* track, working_tracks_) {
        track->Node::InvalidateCache(TimeRange(point_, RATIONAL_MAX), Track::kBlockInput);
      }
    }
  }

  virtual void undo() override
  {
    if (all_tracks_unlocked_) {
      // Optimize by shifting over since we have a constant amount of time being inserted
      if (track_list_->type() == Track::kVideo) {
        track_list_->parent()->ShiftVideoCache(point_ + length_, point_);
      } else if (track_list_->type() == Track::kAudio) {
        track_list_->parent()->ShiftAudioCache(point_ + length_, point_);
      }
    }

    foreach (Track* track, working_tracks_) {
      track->BeginOperation();
    }

    // Remove added gaps
    foreach (auto add_gap, gaps_added_) {
      add_gap.gap->track()->RippleRemoveBlock(add_gap.gap);
      add_gap.gap->setParent(&memory_manager_);
    }

    // Un-split blocks
    if (split_command_) {
      split_command_->undo();
    }

    // Restore original length of gaps
    foreach (Block* gap, gaps_to_extend_) {
      gap->set_length_and_media_out(gap->length() - length_);
    }

    foreach (Track* track, working_tracks_) {
      track->EndOperation();
    }

    if (!all_tracks_unlocked_) {
      foreach (Track* track, working_tracks_) {
        track->Node::InvalidateCache(TimeRange(point_, RATIONAL_MAX), Track::kBlockInput);
      }
    }
  }

private:
  bool prepped_;

  void prep()
  {
    // Determine if all tracks will be affected, which will allow us to make some optimizations
    all_tracks_unlocked_ = true;

    foreach (Track* track, track_list_->GetTracks()) {
      if (track->IsLocked()) {
        all_tracks_unlocked_ = false;
        continue;
      }

      working_tracks_.append(track);
    }

    QVector<Block*> blocks_to_split;
    QVector<Block*> blocks_to_append_gap_to;

    foreach (Track* track, working_tracks_) {
      foreach (Block* b, track->Blocks()) {
        if (b->type() == Block::kGap && b->in() <= point_ && b->out() >= point_) {
          // Found a gap at the location
          gaps_to_extend_.append(b);
          break;
        } else if (b->type() == Block::kClip && b->out() >= point_) {
          if (b->out() > point_) {
            blocks_to_split.append(b);
          }

          blocks_to_append_gap_to.append(b);
          break;
        }
      }
    }

    if (!blocks_to_split.isEmpty()) {
      split_command_ = new BlockSplitPreservingLinksCommand(blocks_to_split, {point_});
      split_command_->redo();
    }

    foreach (Block* block, blocks_to_append_gap_to) {
      GapBlock* gap = new GapBlock();
      gap->set_length_and_media_out(length_);
      gap->setParent(&memory_manager_);
      gaps_added_.append({gap, block});
    }
  }

  TrackList* track_list_;

  rational point_;

  rational length_;

  QVector<Track*> working_tracks_;

  bool all_tracks_unlocked_;

  QVector<Block*> gaps_to_extend_;

  struct AddGap {
    GapBlock* gap;
    Block* before;
  };

  QVector<AddGap> gaps_added_;

  BlockSplitPreservingLinksCommand* split_command_;

  QObject memory_manager_;

};

class TransitionRemoveCommand : public UndoCommand {
public:
  TransitionRemoveCommand(TransitionBlock* block) :
    block_(block)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

  virtual void redo() override
  {
    track_ = block_->track();
    out_block_ = block_->connected_out_block();
    in_block_ = block_->connected_in_block();

    Q_ASSERT(out_block_ || in_block_);

    track_->BeginOperation();

    TimeRange invalidate_range(block_->in(), block_->out());

    if (in_block_) {
      in_block_->set_length_and_media_in(in_block_->length() + block_->in_offset());
    }

    if (out_block_) {
      out_block_->set_length_and_media_out(out_block_->length() + block_->out_offset());
    }

    if (in_block_) {
      Node::DisconnectEdge(in_block_, NodeInput(block_, TransitionBlock::kInBlockInput));
    }

    if (out_block_) {
      Node::DisconnectEdge(out_block_, NodeInput(block_, TransitionBlock::kOutBlockInput));
    }

    track_->RippleRemoveBlock(block_);

    track_->EndOperation();

    track_->Node::InvalidateCache(invalidate_range, Track::kBlockInput);
  }

  virtual void undo() override
  {
    track_->BeginOperation();

    if (in_block_) {
      track_->InsertBlockBefore(block_, in_block_);
    } else {
      track_->InsertBlockAfter(block_, out_block_);
    }

    if (in_block_) {
      Node::ConnectEdge(in_block_, NodeInput(block_, TransitionBlock::kInBlockInput));
    }

    if (out_block_) {
      Node::ConnectEdge(out_block_, NodeInput(block_, TransitionBlock::kOutBlockInput));
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

    track_->Node::InvalidateCache(TimeRange(block_->in(), block_->out()), Track::kBlockInput);
  }

private:
  TransitionBlock* block_;

  Track* track_;

  Block* out_block_;
  Block* in_block_;

};

}

#endif // TIMELINEUNDOABLE_H
