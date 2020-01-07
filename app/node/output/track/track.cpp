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

#include "track.h"

#include <QApplication>
#include <QDebug>
#include <QFontMetrics>

#include "node/block/gap/gap.h"
#include "node/graph.h"

TrackOutput::TrackOutput() :
  track_type_(Timeline::kTrackTypeNone),
  block_invalidate_cache_stack_(0),
  index_(-1),
  locked_(false)
{
  block_input_ = new NodeInputArray("block_in", NodeParam::kAny);
  block_input_->set_is_keyframable(false);
  AddInput(block_input_);
  connect(block_input_, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SLOT(BlockConnected(NodeEdgePtr)));
  connect(block_input_, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SLOT(BlockDisconnected(NodeEdgePtr)));
  connect(block_input_, SIGNAL(SizeChanged(int)), this, SLOT(BlockListSizeChanged(int)));

  muted_input_ = new NodeInput("muted_in", NodeParam::kBoolean);
  muted_input_->set_is_keyframable(false);
  AddInput(muted_input_);

  // Set default height
  track_height_ = GetDefaultTrackHeight();
}

void TrackOutput::set_track_type(const Timeline::TrackType &track_type)
{
  track_type_ = track_type;
}

const Timeline::TrackType& TrackOutput::track_type()
{
  return track_type_;
}

Block::Type TrackOutput::type() const
{
  return kClip;
}

Block *TrackOutput::copy() const
{
  return new TrackOutput();
}

QString TrackOutput::Name() const
{
  return tr("Track");
}

QString TrackOutput::id() const
{
  return "org.olivevideoeditor.Olive.track";
}

QString TrackOutput::Category() const
{
  return tr("Output");
}

QString TrackOutput::Description() const
{
  return tr("Node for representing and processing a single array of Blocks sorted by time. Also represents the end of "
            "a Sequence.");
}

QString TrackOutput::GetTrackName()
{
  if (track_name_.isEmpty()) {
    return GetDefaultTrackName(track_type_, index_);
  }

  return track_name_;
}

const int &TrackOutput::GetTrackHeight() const
{
  return track_height_;
}

void TrackOutput::SetTrackHeight(const int &height)
{
  track_height_ = height;
  emit TrackHeightChanged(track_height_);
}

void TrackOutput::Retranslate()
{
  block_input_->set_name(tr("Blocks"));
  muted_input_->set_name(tr("Muted"));
}

const int &TrackOutput::Index()
{
  return index_;
}

void TrackOutput::SetIndex(const int &index)
{
  index_ = index;
}

Block *TrackOutput::BlockContainingTime(const rational &time) const
{
  foreach (Block* block, block_cache_) {
    if (block->in() < time && block->out() > time) {
      return block;
    } else if (block->out() == time) {
      break;
    }
  }

  return nullptr;
}

Block *TrackOutput::NearestBlockBefore(const rational &time) const
{
  foreach (Block* block, block_cache_) {
    // Blocks are sorted by time, so the first Block who's out point is at/after this time is the correct Block
    if (block->out() >= time) {
      return block;
    }
  }

  return nullptr;
}

Block *TrackOutput::NearestBlockAfter(const rational &time) const
{
  foreach (Block* block, block_cache_) {
    // Blocks are sorted by time, so the first Block after this time is the correct Block
    if (block->in() >= time) {
      return block;
    }
  }

  return nullptr;
}

Block *TrackOutput::BlockAtTime(const rational &time) const
{
  if (IsMuted()) {
    return nullptr;
  }

  foreach (Block* block, block_cache_) {
    if (block
        && block->in() <= time
        && block->out() > time) {
      return block;
    }
  }

  return nullptr;
}

QList<Block *> TrackOutput::BlocksAtTimeRange(const TimeRange &range) const
{
  QList<Block*> list;

  if (IsMuted()) {
    return list;
  }

  foreach (Block* block, block_cache_) {
    if (block
        && block->out() > range.in()
        && block->in() < range.out()) {
      list.append(block);
    }
  }

  return list;
}

const QVector<Block *> &TrackOutput::Blocks() const
{
  return block_cache_;
}

void TrackOutput::InvalidateCache(const rational &start_range, const rational &end_range, NodeInput *from)
{
  Node::InvalidateCache(qMax(start_range, rational(0)), qMin(end_range, track_length()), from);
}

void TrackOutput::InsertBlockBefore(Block* block, Block* after)
{
  InsertBlockAtIndex(block, block_cache_.indexOf(after));
}

void TrackOutput::InsertBlockAfter(Block *block, Block *before)
{
  int before_index = block_cache_.indexOf(before);

  Q_ASSERT(before_index >= 0);

  if (before_index == block_cache_.size() - 1) {
    AppendBlock(block);
  } else {
    InsertBlockAtIndex(block, before_index + 1);
  }
}

void TrackOutput::PrependBlock(Block *block)
{
  InsertBlockAtIndex(block, 0);
}

void TrackOutput::InsertBlockAtIndex(Block *block, int index)
{
  BlockInvalidateCache();

  block_input_->InsertAt(index);
  NodeParam::ConnectEdge(block->output(),
                         block_input_->At(index));

  UnblockInvalidateCache();

  InvalidateCache(block->in(), track_length());
}

void TrackOutput::AppendBlock(Block *block)
{
  BlockInvalidateCache();

  int last_index = block_input_->GetSize();
  block_input_->Append();
  NodeParam::ConnectEdge(block->output(),
                         block_input_->At(last_index));

  UnblockInvalidateCache();

  // Invalidate area that block was added to
  InvalidateCache(block->in(), track_length());
}

void TrackOutput::BlockInvalidateCache()
{
  block_invalidate_cache_stack_++;
}

void TrackOutput::UnblockInvalidateCache()
{
  block_invalidate_cache_stack_--;
}

void TrackOutput::RippleRemoveBlock(Block *block)
{
  BlockInvalidateCache();

  rational remove_in = block->in();

  int index_of_block_to_remove = block_cache_.indexOf(block);

  block_input_->RemoveAt(index_of_block_to_remove);

  UnblockInvalidateCache();

  InvalidateCache(remove_in, track_length());
}

void TrackOutput::ReplaceBlock(Block *old, Block *replace)
{
  Q_ASSERT(old->length() == replace->length());

  BlockInvalidateCache();

  int index_of_old_block = block_cache_.indexOf(old);

  NodeParam::DisconnectEdge(old->output(),
                            block_input_->At(index_of_old_block));

  NodeParam::ConnectEdge(replace->output(),
                         block_input_->At(index_of_old_block));

  UnblockInvalidateCache();

  InvalidateCache(replace->in(), replace->out());
}

TrackOutput *TrackOutput::TrackFromBlock(Block *block)
{
  NodeOutput* output = block->output();

  foreach (NodeEdgePtr edge, output->edges()) {
    Node* n = edge->input()->parentNode();

    if (n->IsTrack()) {
      return static_cast<TrackOutput*>(n);
    }
  }

  return nullptr;
}

const rational &TrackOutput::track_length() const
{
  return track_length_;
}

bool TrackOutput::IsTrack() const
{
  return true;
}

int TrackOutput::GetTrackHeightIncrement()
{
  return qApp->fontMetrics().height() / 2;
}

int TrackOutput::GetDefaultTrackHeight()
{
  return qApp->fontMetrics().height() * 3;
}

int TrackOutput::GetTrackHeightMinimum()
{
  return qApp->fontMetrics().height() * 3 / 2;
}

QString TrackOutput::GetDefaultTrackName(Timeline::TrackType type, int index)
{
  // Starts tracks at 1 rather than 0
  int user_friendly_index = index+1;

  switch (type) {
  case Timeline::kTrackTypeVideo: return tr("Video %1").arg(user_friendly_index);
  case Timeline::kTrackTypeAudio: return tr("Audio %1").arg(user_friendly_index);
  case Timeline::kTrackTypeSubtitle: return tr("Subtitle %1").arg(user_friendly_index);
  case Timeline::kTrackTypeNone:
  case Timeline::kTrackTypeCount:
    break;
  }

  return tr("Track %1").arg(user_friendly_index);
}

bool TrackOutput::IsMuted() const
{
  return muted_input_->get_standard_value().toBool();
}

bool TrackOutput::IsLocked() const
{
  return locked_;
}

void TrackOutput::SetTrackName(const QString &name)
{
  track_name_ = name;
}

void TrackOutput::SetMuted(bool e)
{
  muted_input_->set_standard_value(e);
  InvalidateCache(0, track_length());
}

void TrackOutput::SetLocked(bool e)
{
  locked_ = e;
}

void TrackOutput::UpdateInOutFrom(int index)
{
  Q_ASSERT(index >= 0);
  Q_ASSERT(index < block_cache_.size());

  rational new_track_length;

  // Find block just before this one to find the last out point
  for (int i=index-1;i>=0;i--) {
    Block* b = block_cache_.at(i);

    if (b) {
      new_track_length = b->out();
      break;
    }
  }

  for (int i=index;i<block_cache_.size();i++) {
    Block* b = block_cache_.at(i);

    if (b) {
      // Set in
      b->set_in(new_track_length);

      // Set out
      new_track_length += b->length();
      b->set_out(new_track_length);

      emit b->Refreshed();
    }
  }

  // Update track length
  if (new_track_length != track_length_) {
    track_length_ = new_track_length;
    emit TrackLengthChanged();
  }
}

void TrackOutput::UpdatePreviousAndNextOfIndex(int index)
{
  Block* ref = block_cache_.at(index);

  Block* previous = nullptr;
  Block* next = nullptr;

  // Find previous
  for (int i=index-1;i>=0;i--) {
    previous = block_cache_.at(i);

    if (previous) {
      break;
    }
  }

  // Find next
  for (int i=index+1;i<block_cache_.size();i++) {
    next = block_cache_.at(i);

    if (next) {
      break;
    }
  }

  if (ref) {
    // Link blocks together
    ref->set_previous(previous);
    ref->set_next(next);

    if (previous)
      previous->set_next(ref);

    if (next)
      next->set_previous(ref);
  } else {
    // Link previous and next together
    if (previous)
      previous->set_next(next);

    if (next)
      next->set_previous(previous);
  }
}

void TrackOutput::BlockConnected(NodeEdgePtr edge)
{
  int block_index = block_input_->IndexOfSubParameter(edge->input());

  Q_ASSERT(block_index >= 0);

  Node* connected_node = edge->output()->parentNode();
  Block* connected_block = connected_node->IsBlock() ? static_cast<Block*>(connected_node) : nullptr;
  block_cache_.replace(block_index, connected_block);
  UpdatePreviousAndNextOfIndex(block_index);
  UpdateInOutFrom(block_index);

  if (connected_block) {
    connect(connected_block, SIGNAL(LengthChanged(const rational&)), this, SLOT(BlockLengthChanged()));

    emit BlockAdded(connected_block);
  }
}

void TrackOutput::BlockDisconnected(NodeEdgePtr edge)
{
  int block_index = block_input_->IndexOfSubParameter(edge->input());

  Q_ASSERT(block_index >= 0);

  block_cache_.replace(block_index, nullptr);
  UpdatePreviousAndNextOfIndex(block_index);
  UpdateInOutFrom(block_index);

  Node* connected_node = edge->output()->parentNode();
  Block* connected_block = connected_node->IsBlock() ? static_cast<Block*>(connected_node) : nullptr;
  if (connected_block) {
    disconnect(connected_block, SIGNAL(LengthChanged(const rational&)), this, SLOT(BlockLengthChanged()));

    // Update previous and next references
    emit BlockRemoved(connected_block);
  }
}

void TrackOutput::BlockListSizeChanged(int size)
{
  int old_size = block_cache_.size();

  block_cache_.resize(size);

  // Fill new slots with nullptr
  for (int i=old_size;i<size;i++) {
    block_cache_.replace(i, nullptr);
  }
}

void TrackOutput::BlockLengthChanged()
{
  // Assumes sender is a Block
  Block* b = static_cast<Block*>(sender());

  int index = block_cache_.indexOf(b);

  Q_ASSERT(index >= 0);

  UpdateInOutFrom(index);
}
