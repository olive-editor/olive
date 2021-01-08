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

#include "track.h"

#include <QApplication>
#include <QDebug>
#include <QFontMetrics>

#include "node/block/gap/gap.h"
#include "node/graph.h"

namespace olive {

const double TrackOutput::kTrackHeightDefault = 3.0;
const double TrackOutput::kTrackHeightMinimum = 1.5;
const double TrackOutput::kTrackHeightInterval = 0.5;

TrackOutput::TrackOutput() :
  track_type_(Timeline::kTrackTypeNone),
  index_(-1),
  locked_(false)
{
  block_input_ = new NodeInput(this, QStringLiteral("block_in"), NodeValue::kNone);
  block_input_->SetKeyframable(false);
  connect(block_input_, &NodeInput::InputConnected, this, &TrackOutput::BlockConnected);
  connect(block_input_, &NodeInput::InputDisconnected, this, &TrackOutput::BlockDisconnected);

  // Since blocks are time based, we can handle the invalidate timing a little more intelligently
  // on our end
  IgnoreConnectionSignalsFrom(block_input_);

  muted_input_ = new NodeInput(this, QStringLiteral("muted_in"), NodeValue::kBoolean);
  muted_input_->SetKeyframable(false);
  connect(muted_input_, &NodeInput::ValueChanged, this, &TrackOutput::MutedInputValueChanged);

  // Set default height
  track_height_ = kTrackHeightDefault;
}

TrackOutput::~TrackOutput()
{
  DisconnectAll();
}

void TrackOutput::set_track_type(const Timeline::TrackType &track_type)
{
  track_type_ = track_type;
}

const Timeline::TrackType& TrackOutput::track_type() const
{
  return track_type_;
}

Node *TrackOutput::copy() const
{
  return new TrackOutput();
}

QString TrackOutput::Name() const
{
  return tr("Track");
}

QString TrackOutput::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.track");
}

QVector<Node::CategoryID> TrackOutput::Category() const
{
  return {kCategoryTimeline};
}

QString TrackOutput::Description() const
{
  return tr("Node for representing and processing a single array of Blocks sorted by time. Also represents the end of "
            "a Sequence.");
}

const double &TrackOutput::GetTrackHeight() const
{
  return track_height_;
}

void TrackOutput::SetTrackHeight(const double &height)
{
  track_height_ = height;
  emit TrackHeightChangedInPixels(GetTrackHeightInPixels());
}

void TrackOutput::LoadInternal(QXmlStreamReader *reader, XMLNodeData &)
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("height")) {
      SetTrackHeight(reader->readElementText().toDouble());
    } else {
      reader->skipCurrentElement();
    }
  }
}

void TrackOutput::SaveInternal(QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("height"), QString::number(GetTrackHeight()));
}

void TrackOutput::Retranslate()
{
  Node::Retranslate();

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

  emit IndexChanged(index);
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

Block *TrackOutput::NearestBlockBeforeOrAt(const rational &time) const
{
  foreach (Block* block, block_cache_) {
    // Blocks are sorted by time, so the first Block who's out point is at/after this time is the correct Block
    if (block->out() > time) {
      return block;
    }
  }

  return nullptr;
}

Block *TrackOutput::NearestBlockAfterOrAt(const rational &time) const
{
  foreach (Block* block, block_cache_) {
    // Blocks are sorted by time, so the first Block after this time is the correct Block
    if (block->in() >= time) {
      return block;
    }
  }

  return nullptr;
}

Block *TrackOutput::NearestBlockAfter(const rational &time) const
{
  foreach (Block* block, block_cache_) {
    // Blocks are sorted by time, so the first Block after this time is the correct Block
    if (block->in() > time) {
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
      if (block->is_enabled()) {
        return block;
      } else {
        break;
      }
    }
  }

  return nullptr;
}

QVector<Block *> TrackOutput::BlocksAtTimeRange(const TimeRange &range) const
{
  QVector<Block*> list;

  if (IsMuted()) {
    return list;
  }

  foreach (Block* block, block_cache_) {
    if (block
        && block->is_enabled()
        && block->out() > range.in()
        && block->in() < range.out()) {
      list.append(block);
    }
  }

  return list;
}

void TrackOutput::InvalidateCache(const TimeRange& range, const InputConnection& from)
{
  TimeRange limited;

  Block* b;

  if (from.input == block_input_
      && from.element >= 0
      && (b = dynamic_cast<Block*>(from.input->GetConnectedNode(from.element)))) {
    // Limit the range signal to the corresponding block
    if (range.out() <= b->in() || range.in() >= b->out()) {
      return;
    }

    limited = TimeRange(qMax(range.in(), b->in()), qMin(range.out(), b->out()));
  } else {
    limited = TimeRange(qMax(range.in(), rational(0)), qMin(range.out(), track_length()));
  }

  Node::InvalidateCache(limited, from);
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
  BeginOperation();

  block_input_->ArrayPrepend();
  Node::ConnectEdge(block, block_input_, 0);

  EndOperation();

  // Everything has shifted at this point
  Node::InvalidateCache(TimeRange(0, track_length()), InputConnection());
}

void TrackOutput::InsertBlockAtIndex(Block *block, int index)
{
  BeginOperation();

  int insert_index = GetInputIndexFromCacheIndex(index);
  block_input_->ArrayInsert(insert_index);
  Node::ConnectEdge(block, block_input_, insert_index);

  EndOperation();

  Node::InvalidateCache(TimeRange(block->in(), track_length()));
}

void TrackOutput::AppendBlock(Block *block)
{
  BeginOperation();

  block_input_->ArrayAppend();
  Node::ConnectEdge(block, block_input_, block_input_->ArraySize() - 1);

  EndOperation();

  // Invalidate area that block was added to
  Node::InvalidateCache(TimeRange(block->in(), track_length()));
}

void TrackOutput::RippleRemoveBlock(Block *block)
{
  BeginOperation();

  rational remove_in = block->in();
  rational remove_out = block->out();

  block_input_->ArrayRemove(GetInputIndexFromCacheIndex(block));

  EndOperation();

  Node::InvalidateCache(TimeRange(remove_in, qMax(track_length(), remove_out)));
}

void TrackOutput::ReplaceBlock(Block *old, Block *replace)
{
  BeginOperation();

  int index_of_old_block = GetInputIndexFromCacheIndex(old);

  DisconnectEdge(old, block_input_, index_of_old_block);

  ConnectEdge(replace, block_input_, index_of_old_block);

  EndOperation();

  if (old->length() == replace->length()) {
    Node::InvalidateCache(TimeRange(replace->in(), replace->out()));
  } else {
    Node::InvalidateCache(TimeRange(replace->in(), RATIONAL_MAX));
  }
}

TrackOutput *TrackOutput::TrackFromBlock(const Block *block)
{
  foreach (const InputConnection& conn, block->edges()) {
    TrackOutput* track = dynamic_cast<TrackOutput*>(conn.input->parent());

    if (track) {
      return track;
    }
  }

  return nullptr;
}

const rational &TrackOutput::track_length() const
{
  return track_length_;
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
  return muted_input_->GetStandardValue().toBool();
}

bool TrackOutput::IsLocked() const
{
  return locked_;
}

NodeInput *TrackOutput::block_input() const
{
  return block_input_;
}

void TrackOutput::Hash(QCryptographicHash &hash, const rational &time) const
{
  Block* b = BlockAtTime(time);

  // Defer to block at this time, don't add any of our own information to the hash
  if (b) {
    b->Hash(hash, time);
  }
}

void TrackOutput::SetMuted(bool e)
{
  muted_input_->SetStandardValue(e);
  Node::InvalidateCache(TimeRange(0, track_length()));
}

void TrackOutput::SetLocked(bool e)
{
  locked_ = e;
}

void TrackOutput::UpdateInOutFrom(int index)
{
  // Find block just before this one to find the last out point
  rational last_out = (index == 0) ? 0 : block_cache_.at(index - 1)->out();

  // Iterate through all blocks updating their in/outs
  for (int i=index; i<block_cache_.size(); i++) {
    Block* b = block_cache_.at(i);

    b->set_in(last_out);

    last_out += b->length();

    b->set_out(last_out);

    emit b->Refreshed();
  }

  // Update track length
  SetLengthInternal(last_out);
}

int TrackOutput::GetInputIndexFromCacheIndex(int cache_index)
{
  return GetInputIndexFromCacheIndex(block_cache_.at(cache_index));
}

int TrackOutput::GetInputIndexFromCacheIndex(Block *block)
{
  for (int i=0; i<block_input_->ArraySize(); i++) {
    if (block_input_->GetConnectedNode(i) == block) {
      return i;
    }
  }

  return -1;
}

void TrackOutput::SetLengthInternal(const rational &r, bool invalidate)
{
  if (r != track_length_) {
    TimeRange invalidate_range(track_length_, r);

    track_length_ = r;
    emit TrackLengthChanged();

    if (invalidate) {
      Node::InvalidateCache(invalidate_range);
    }
  }
}

void TrackOutput::BlockConnected(Node *node, int element)
{
  if (element == -1) {
    // User has replaced the entire array, we will invalidate everything
    InputConnectionChanged(node, element);
    return;
  }

  // Check if a block was connected, if not, ignore
  Block* block = dynamic_cast<Block*>(node);

  if (!block) {
    return;
  }

  Block *previous = nullptr, *next = nullptr;

  for (int i=element-1; i>=0; i--) {
    // Find previous block
    previous = dynamic_cast<Block*>(block_input_->GetConnectedNode(i));

    if (previous) {
      break;
    }
  }

  // Find cache index
  int cache_index;
  if (previous) {
    // Insert block just after the previous block we found
    cache_index = block_cache_.indexOf(previous) + 1;

    // Use current previous' next as our next
    next = previous->next();
  } else {
    // Didn't find a previous, so insert block at 0 (prepend it)
    cache_index = 0;

    if (!block_cache_.isEmpty()) {
      next = block_cache_.first();
    }
  }

  // Insert at index
  block_cache_.insert(cache_index, block);

  // Update previous/next
  if (previous) {
    previous->set_next(block);
    block->set_previous(previous);
  }

  if (next) {
    block->set_next(next);
    next->set_previous(block);
  }

  // Update ins/outs
  UpdateInOutFrom(cache_index);

  // Connect to the block
  connect(block, &Block::LengthChanged, this, &TrackOutput::BlockLengthChanged);

  // Invalidate cache now that block should have an in point
  Node::InvalidateCache(TimeRange(block->in(), track_length()));

  // Emit block added signal
  emit BlockAdded(block);
}

void TrackOutput::BlockDisconnected(Node* node, int element)
{
  if (element == -1) {
    // User has replaced the entire array, we will invalidate everything
    InputConnectionChanged(node, element);
    return;
  }

  Block* b = dynamic_cast<Block*>(node);

  if (!b) {
    return;
  }

  TimeRange invalidate_range(b->in(), track_length());

  // FIXME: What happens if a user connects the same block twice? This must be addressed in the
  //        upcoming timeline rewrite.
  block_cache_.removeOne(b);

  Block* previous = b->previous();
  Block* next = b->next();

  if (previous) {
    previous->set_next(next);
  }

  if (next) {
    next->set_previous(previous);
  }

  b->set_previous(nullptr);
  b->set_next(nullptr);

  if (next) {
    UpdateInOutFrom(block_cache_.indexOf(next));
  } else if (block_cache_.isEmpty()) {
    SetLengthInternal(rational());
  } else {
    SetLengthInternal(block_cache_.last()->out());
  }

  disconnect(b, &Block::LengthChanged, this, &TrackOutput::BlockLengthChanged);

  emit BlockRemoved(b);

  Node::InvalidateCache(invalidate_range);
}

void TrackOutput::BlockLengthChanged()
{
  // Assumes sender is a Block
  Block* b = static_cast<Block*>(sender());

  rational old_out = b->out();

  UpdateInOutFrom(block_cache_.indexOf(b));

  rational new_out = b->out();

  TimeRange invalidate_region(qMin(old_out, new_out), track_length());

  Node::InvalidateCache(invalidate_region);
}

void TrackOutput::MutedInputValueChanged()
{
  emit MutedChanged(IsMuted());
}

}
