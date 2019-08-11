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

#include "timeline.h"


TimelineOutput::TimelineOutput() :
  first_block_(nullptr),
  current_block_(this),
  attached_timeline_(nullptr)
{
}

Block::Type TimelineOutput::type()
{
  return kEnd;
}

QString TimelineOutput::Name()
{
  return tr("Timeline");
}

QString TimelineOutput::id()
{
  return "org.olivevideoeditor.Olive.timeline";
}

QString TimelineOutput::Category()
{
  return tr("Output");
}

QString TimelineOutput::Description()
{
  return tr("Node for communicating between a Timeline panel and the node graph. Also represents the end of a"
            "Sequence.");
}

#include "project/item/sequence/sequence.h"

void TimelineOutput::AttachTimeline(TimelinePanel *timeline)
{
  if (attached_timeline_ != nullptr) {
    disconnect(attached_timeline_, SIGNAL(RequestInsertBlock(Block*, int)), this, SLOT(InsertBlock(Block*, int)));

    attached_timeline_->Clear();
  }

  attached_timeline_ = timeline;

  if (attached_timeline_ != nullptr) {
    attached_timeline_->Clear();

    // FIXME: TEST CODE ONLY
    attached_timeline_->SetTimebase(rational(1001, 30000));
    // END TEST CODE

    Block* previous_block = attached_block();
    while (previous_block != nullptr) {

      // FIXME: Dynamic cast is a dumb way of doing this
      ClipBlock* clip_block = dynamic_cast<ClipBlock*>(previous_block);

      if (clip_block != nullptr) {
        attached_timeline_->AddClip(clip_block);
      }

      previous_block = previous_block->previous();
    }

    connect(attached_timeline_, SIGNAL(RequestInsertBlock(Block*, int)), this, SLOT(InsertBlock(Block*, int)));
  }
}

rational TimelineOutput::length()
{
  return 0;
}

void TimelineOutput::Refresh()
{
  QVector<Block*> detect_attached_blocks;

  Block* previous = attached_block();
  while (previous != nullptr) {
    detect_attached_blocks.prepend(previous);

    if (attached_timeline_ != nullptr && !block_cache_.contains(previous)) {
      // FIXME: Remove the need to cast this
      attached_timeline_->AddClip(static_cast<ClipBlock*>(previous));
    }

    previous = previous->previous();
  }

  if (attached_timeline_ != nullptr) {
    foreach (Block* b, block_cache_) {
      if (!detect_attached_blocks.contains(b)) {
        attached_timeline_->RemoveClip(static_cast<ClipBlock*>(b));
      }
    }
  }

  block_cache_ = detect_attached_blocks;

  Block::Refresh();
}

void TimelineOutput::Process(const rational &time)
{
  // Run default node processing
  Block::Process(time);

  // This node representso the end of the timeline, so being beyond its in point is considered the end of the sequence
  if (time >= in()) {
    texture_output()->set_value(0);
    current_block_ = this;
    return;
  }

  // If we're here, we need to find the current clip to display
  // attached_block() is guaranteed to not be nullptr if we didn't return before
  current_block_ = attached_block();

  // If the time requested is an earlier Block, traverse earlier until we find it
  while (time < current_block_->in()) {
    current_block_ = current_block_->previous();
  }

  // If the time requested is in a later Block, traverse later
  while (time >= current_block_->out()) {
    current_block_ = current_block_->next();
  }

  // At this point, we must have found the correct block so we use its texture output to produce the image
  texture_output()->set_value(current_block_->texture_output()->get_value(time));
}

Block *TimelineOutput::first_block()
{
  if (block_cache_.isEmpty()) {
    return nullptr;
  }

  return block_cache_.first();
}

Block *TimelineOutput::attached_block()
{
  return ValueToPtr<Block>(previous_input()->get_value(0));
}

void TimelineOutput::InsertBlock(Block *block, int index)
{
  // Add node and its connected nodes to graph
  NodeGraph* graph = static_cast<NodeGraph*>(parent());
  graph->AddNode(block);

  // Add all of Block's dependencies
  QList<Node*> block_dependencies = block->GetDependencies();
  foreach (Node* dep, block_dependencies) {
    graph->AddNode(dep);
  }

  if (block_cache_.isEmpty()) {

    // If there are no blocks connected, the index doesn't matter. Just connect it.
    Block::ConnectBlocks(block, this);

  } else if (index == 0) {

    // If the index is 0, it goes at the very beginning
    Block::ConnectBlocks(block, block_cache_.first());

  } else {

    // Otherwise, the block goes between two other blocks somehow
    Block* before;
    Block* after;

    if (index < block_cache_.size()) {
      // The block goes somewhere in between some set of two blocks
      before = block_cache_.at(index - 1);
      after = block_cache_.at(index);
    } else {
      // The block goes at the very end
      before = block_cache_.last();
      after = this;
    }

    // Connect blocks correctly
    Block::DisconnectBlocks(before, after);
    Block::ConnectBlocks(before, block);
    Block::ConnectBlocks(block, after);

  }
}
