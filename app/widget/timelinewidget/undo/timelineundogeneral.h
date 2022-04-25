/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef TIMELINEUNDOGENERAL_H
#define TIMELINEUNDOGENERAL_H

#include "config/config.h"
#include "node/block/clip/clip.h"
#include "node/block/gap/gap.h"
#include "node/output/track/track.h"
#include "node/output/track/tracklist.h"
#include "node/output/viewer/viewer.h"
#include "node/project/sequence/sequence.h"
#include "timelineundosplit.h"

namespace olive {

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

protected:
  virtual void redo() override;
  virtual void undo() override;

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

  virtual Project* GetRelevantProject() const
  {
    return block_->project();
  }

protected:
  virtual void redo();
  virtual void undo();

private:
  Block* block_;
  rational old_length_;
  rational new_length_;

};

class BlockSetMediaInCommand : public UndoCommand {
public:
  BlockSetMediaInCommand(ClipBlock* block, rational new_media_in) :
    block_(block),
    new_media_in_(new_media_in)
  {
  }

  virtual Project* GetRelevantProject() const
  {
    return block_->project();
  }

protected:
  virtual void redo();
  virtual void undo();

private:
  ClipBlock* block_;
  rational old_media_in_;
  rational new_media_in_;

};

class TimelineAddTrackCommand : public UndoCommand {
public:
  TimelineAddTrackCommand(TrackList *timeline) :
    TimelineAddTrackCommand(timeline, Config::Current()[QStringLiteral("AutoMergeTracks")].toBool())
  {
  }

  TimelineAddTrackCommand(TrackList *timeline, bool automerge_tracks);

  static Track* RunImmediately(TrackList *timeline)
  {
    TimelineAddTrackCommand c(timeline);
    c.redo();
    return c.track();
  }

  static Track* RunImmediately(TrackList *timeline, bool automerge)
  {
    TimelineAddTrackCommand c(timeline, automerge);
    c.redo();
    return c.track();
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

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  TrackList* timeline_;

  Track* track_;
  Node* merge_;
  NodeInput base_;
  NodeInput blend_;

  NodeInput direct_;

  MultiUndoCommand* position_command_;

  QObject memory_manager_;

};

class TimelineRemoveTrackCommand : public UndoCommand
{
public:
  TimelineRemoveTrackCommand(Track *track) :
    track_(track),
    remove_command_(nullptr)
  {}

  virtual ~TimelineRemoveTrackCommand()
  {
    delete remove_command_;
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

protected:
  virtual void prepare() override;

  virtual void redo() override;

  virtual void undo() override;

private:
  Track *track_;

  TrackList *list_;

  int index_;

  UndoCommand *remove_command_;

};

class TransitionRemoveCommand : public UndoCommand {
public:
  TransitionRemoveCommand(TransitionBlock* block, bool remove_from_graph) :
    block_(block),
    remove_from_graph_(remove_from_graph),
    remove_command_(nullptr)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  TransitionBlock* block_;

  Track* track_;

  Block* out_block_;
  Block* in_block_;

  bool remove_from_graph_;
  UndoCommand* remove_command_;

};

class TrackReplaceBlockWithGapCommand : public UndoCommand {
public:
  TrackReplaceBlockWithGapCommand(Track* track, Block* block, bool handle_transitions = true, bool handle_invalidations = true) :
    track_(track),
    block_(block),
    existing_gap_(nullptr),
    existing_merged_gap_(nullptr),
    our_gap_(nullptr),
    handle_transitions_(handle_transitions),
    handle_invalidations_(handle_invalidations)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return block_->project();
  }

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  void CreateRemoveTransitionCommandIfNecessary(bool next);

  Track* track_;
  Block* block_;

  GapBlock* existing_gap_;
  GapBlock* existing_merged_gap_;
  bool existing_gap_precedes_;
  GapBlock* our_gap_;

  bool handle_transitions_;
  bool handle_invalidations_;

  QObject memory_manager_;

  QVector<TransitionRemoveCommand*> transition_remove_commands_;

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

protected:
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

class TrackListInsertGaps : public UndoCommand {
public:
  TrackListInsertGaps(TrackList* track_list, const rational& point, const rational& length) :
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

protected:
  virtual void prepare() override;

  virtual void redo() override;

  virtual void undo() override;

private:
  TrackList* track_list_;

  rational point_;

  rational length_;

  QVector<Track*> working_tracks_;

  bool all_tracks_unlocked_;

  QVector<Block*> gaps_to_extend_;

  struct AddGap {
    GapBlock* gap;
    Block* before;
    Track* track;
  };

  QVector<AddGap> gaps_added_;

  BlockSplitPreservingLinksCommand* split_command_;

  QObject memory_manager_;

};

class NodeBeginOperationCommand : public UndoCommand
{
public:
  NodeBeginOperationCommand(Node *node) :
    node_(node)
  {}

  virtual Project* GetRelevantProject() const override
  {
    return node_->project();
  }

protected:
  virtual void redo() override
  {
    node_->BeginOperation();
  }

  virtual void undo() override
  {
    node_->EndOperation();
  }

private:
  Node *node_;

};

class NodeEndOperationCommand : public UndoCommand
{
public:
  NodeEndOperationCommand(Node *node) :
    node_(node)
  {}

  virtual Project* GetRelevantProject() const override
  {
    return node_->project();
  }

protected:
  virtual void redo() override
  {
    node_->EndOperation();
  }

  virtual void undo() override
  {
    node_->BeginOperation();
  }

private:
  Node *node_;

};

}

#endif // TIMELINEUNDOGENERAL_H
