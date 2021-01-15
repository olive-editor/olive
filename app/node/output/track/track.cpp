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

const double Track::kTrackHeightDefault = 3.0;
const double Track::kTrackHeightMinimum = 1.5;
const double Track::kTrackHeightInterval = 0.5;

Track::Track() :
  track_type_(Track::kNone),
  index_(-1),
  locked_(false)
{
  block_input_ = new NodeInput(this, QStringLiteral("block_in"), NodeValue::kNone);
  block_input_->SetKeyframable(false);
  connect(block_input_, &NodeInput::InputConnected, this, &Track::BlockConnected);
  connect(block_input_, &NodeInput::InputDisconnected, this, &Track::BlockDisconnected);

  // Since blocks are time based, we can handle the invalidate timing a little more intelligently
  // on our end
  IgnoreInvalidationsFrom(block_input_);

  muted_input_ = new NodeInput(this, QStringLiteral("muted_in"), NodeValue::kBoolean);
  muted_input_->SetKeyframable(false);
  connect(muted_input_, &NodeInput::ValueChanged, this, &Track::MutedInputValueChanged);

  // Set default height
  track_height_ = kTrackHeightDefault;
}

Track::~Track()
{
  DisconnectAll();
}

void Track::set_type(const Type &track_type)
{
  track_type_ = track_type;
}

const Track::Type& Track::type() const
{
  return track_type_;
}

Node *Track::copy() const
{
  return new Track();
}

QString Track::Name() const
{
  return tr("Track");
}

QString Track::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.track");
}

QVector<Node::CategoryID> Track::Category() const
{
  return {kCategoryTimeline};
}

QString Track::Description() const
{
  return tr("Node for representing and processing a single array of Blocks sorted by time. Also represents the end of "
            "a Sequence.");
}

TimeRange Track::InputTimeAdjustment(NodeInput *input, int element, const TimeRange &input_time) const
{
  if (input == block_input_ && element >= 0) {
    int cache_index = GetCacheIndexFromArrayIndex(element);

    return TransformRangeForBlock(blocks_.at(cache_index), input_time);
  }

  return Node::InputTimeAdjustment(input, element, input_time);
}

TimeRange Track::OutputTimeAdjustment(NodeInput *input, int element, const TimeRange &input_time) const
{
  if (input == block_input_ && element >= 0) {
    int cache_index = GetCacheIndexFromArrayIndex(element);
    const rational& block_in = blocks_.at(cache_index)->in();

    return input_time + block_in;
  }

  return Node::OutputTimeAdjustment(input, element, input_time);
}

TimeRange Track::TransformRangeForBlock(Block *block, const TimeRange &range)
{
  return range - block->in();
}

const double &Track::GetTrackHeight() const
{
  return track_height_;
}

void Track::SetTrackHeight(const double &height)
{
  track_height_ = height;
  emit TrackHeightChangedInPixels(GetTrackHeightInPixels());
}

void Track::LoadInternal(QXmlStreamReader *reader, XMLNodeData &)
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("height")) {
      SetTrackHeight(reader->readElementText().toDouble());
    } else {
      reader->skipCurrentElement();
    }
  }
}

void Track::SaveInternal(QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("height"), QString::number(GetTrackHeight()));
}

void Track::Retranslate()
{
  Node::Retranslate();

  block_input_->set_name(tr("Blocks"));
  muted_input_->set_name(tr("Muted"));
}

void Track::SetIndex(const int &index)
{
  index_ = index;

  emit IndexChanged(index);
}

Block *Track::BlockContainingTime(const rational &time) const
{
  foreach (Block* block, blocks_) {
    if (block->in() < time && block->out() > time) {
      return block;
    } else if (block->out() == time) {
      break;
    }
  }

  return nullptr;
}

Block *Track::NearestBlockBefore(const rational &time) const
{
  foreach (Block* block, blocks_) {
    // Blocks are sorted by time, so the first Block who's out point is at/after this time is the correct Block
    if (block->out() >= time) {
      return block;
    }
  }

  return nullptr;
}

Block *Track::NearestBlockBeforeOrAt(const rational &time) const
{
  foreach (Block* block, blocks_) {
    // Blocks are sorted by time, so the first Block who's out point is at/after this time is the correct Block
    if (block->out() > time) {
      return block;
    }
  }

  return nullptr;
}

Block *Track::NearestBlockAfterOrAt(const rational &time) const
{
  foreach (Block* block, blocks_) {
    // Blocks are sorted by time, so the first Block after this time is the correct Block
    if (block->in() >= time) {
      return block;
    }
  }

  return nullptr;
}

Block *Track::NearestBlockAfter(const rational &time) const
{
  foreach (Block* block, blocks_) {
    // Blocks are sorted by time, so the first Block after this time is the correct Block
    if (block->in() > time) {
      return block;
    }
  }

  return nullptr;
}

Block *Track::BlockAtTime(const rational &time) const
{
  if (IsMuted()) {
    return nullptr;
  }

  for (int i=0; i<blocks_.size(); i++) {
    Block* block = blocks_.at(i);

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

QVector<Block *> Track::BlocksAtTimeRange(const TimeRange &range) const
{
  QVector<Block*> list;

  if (IsMuted()) {
    return list;
  }

  foreach (Block* block, blocks_) {
    if (block
        && block->is_enabled()
        && block->out() > range.in()
        && block->in() < range.out()) {
      list.append(block);
    }
  }

  return list;
}

void Track::InvalidateCache(const TimeRange& range, const InputConnection& from)
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
    limited = TimeRange(qMax(range.in(), rational(0)), qMin(range.out(), last_invalidated_length_));
    last_invalidated_length_ = track_length();
  }

  Node::InvalidateCache(limited, from);
}

void Track::InsertBlockBefore(Block* block, Block* after)
{
  if (!after) {
    AppendBlock(block);
  } else {
    InsertBlockAtIndex(block, blocks_.indexOf(after));
  }
}

void Track::InsertBlockAfter(Block *block, Block *before)
{
  if (!before) {
    PrependBlock(block);
  } else {
    int before_index = blocks_.indexOf(before);

    Q_ASSERT(before_index >= 0);

    if (before_index == blocks_.size() - 1) {
      AppendBlock(block);
    } else {
      InsertBlockAtIndex(block, before_index + 1);
    }
  }
}

void Track::PrependBlock(Block *block)
{
  BeginOperation();

  block_input_->ArrayPrepend();
  Node::ConnectEdge(block, block_input_, 0);

  EndOperation();

  // Everything has shifted at this point
  InvalidateCache(TimeRange(0, track_length()));
}

void Track::InsertBlockAtIndex(Block *block, int index)
{
  BeginOperation();

  int insert_index = GetArrayIndexFromCacheIndex(index);
  block_input_->ArrayInsert(insert_index);
  Node::ConnectEdge(block, block_input_, insert_index);

  EndOperation();

  InvalidateCache(TimeRange(block->in(), track_length()));
}

void Track::AppendBlock(Block *block)
{
  BeginOperation();

  block_input_->ArrayAppend();
  Node::ConnectEdge(block, block_input_, block_input_->ArraySize() - 1);

  EndOperation();

  // Invalidate area that block was added to
  InvalidateCache(TimeRange(block->in(), track_length()));
}

void Track::RippleRemoveBlock(Block *block)
{
  BeginOperation();

  rational remove_in = block->in();
  rational remove_out = block->out();

  block_input_->ArrayRemove(GetArrayIndexFromBlock(block));

  EndOperation();

  InvalidateCache(TimeRange(remove_in, qMax(track_length(), remove_out)));
}

void Track::ReplaceBlock(Block *old, Block *replace)
{
  BeginOperation();

  int index_of_old_block = GetArrayIndexFromBlock(old);

  DisconnectEdge(old, block_input_, index_of_old_block);

  ConnectEdge(replace, block_input_, index_of_old_block);

  EndOperation();

  if (old->length() == replace->length()) {
    InvalidateCache(TimeRange(replace->in(), replace->out()));
  } else {
    InvalidateCache(TimeRange(replace->in(), RATIONAL_MAX));
  }
}

const rational &Track::track_length() const
{
  return track_length_;
}

QString Track::GetDefaultTrackName(Track::Type type, int index)
{
  // Starts tracks at 1 rather than 0
  int user_friendly_index = index+1;

  switch (type) {
  case Track::kVideo: return tr("Video %1").arg(user_friendly_index);
  case Track::kAudio: return tr("Audio %1").arg(user_friendly_index);
  case Track::kSubtitle: return tr("Subtitle %1").arg(user_friendly_index);
  case Track::kNone:
  case Track::kCount:
    break;
  }

  return tr("Track %1").arg(user_friendly_index);
}

bool Track::IsMuted() const
{
  return muted_input_->GetStandardValue().toBool();
}

bool Track::IsLocked() const
{
  return locked_;
}

NodeInput *Track::block_input() const
{
  return block_input_;
}

void Track::Hash(QCryptographicHash &hash, const rational &time) const
{
  Block* b = BlockAtTime(time);

  // Defer to block at this time, don't add any of our own information to the hash
  if (b) {
    b->Hash(hash, time);
  }
}

void Track::SetMuted(bool e)
{
  muted_input_->SetStandardValue(e);
  InvalidateCache(TimeRange(0, track_length()));
}

void Track::SetLocked(bool e)
{
  locked_ = e;
}

void Track::UpdateInOutFrom(int index)
{
  // Find block just before this one to find the last out point
  rational last_out = (index == 0) ? 0 : blocks_.at(index - 1)->out();

  // Iterate through all blocks updating their in/outs
  for (int i=index; i<blocks_.size(); i++) {
    Block* b = blocks_.at(i);

    b->set_in(last_out);

    last_out += b->length();

    b->set_out(last_out);

    b->set_index(i);
  }

  emit BlocksRefreshed();

  // Update track length
  SetLengthInternal(last_out);
}

int Track::GetArrayIndexFromBlock(Block *block) const
{
  return block_array_indexes_.at(blocks_.indexOf(block));
}

int Track::GetArrayIndexFromCacheIndex(int index) const
{
  return block_array_indexes_.at(index);
}

int Track::GetCacheIndexFromArrayIndex(int index) const
{
  return block_array_indexes_.indexOf(index);
}

void Track::SetLengthInternal(const rational &r, bool invalidate)
{
  if (r != track_length_) {
    // TimeRange will automatically normalize so that the shorter number is the in and the longer
    // is the out
    TimeRange invalidate_range(track_length_, r);

    track_length_ = r;
    last_invalidated_length_ = qMax(last_invalidated_length_, track_length_);
    emit TrackLengthChanged();

    if (invalidate) {
      InvalidateCache(invalidate_range);
    }
  }
}

void Track::BlockConnected(Node *node, int element)
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

  // Determine where in the cache this block will be
  int cache_index = -1;
  Block *previous = nullptr, *next = nullptr;

  for (int i=element+1; i<block_input_->ArraySize(); i++) {
    // Find next block because this will be the index that we want to insert at
    cache_index = GetCacheIndexFromArrayIndex(i);

    if (cache_index >= 0) {
      next = blocks_.at(cache_index);
      break;
    }
  }

  // If there was no next, this will be inserted at the end
  if (cache_index == -1) {
    cache_index = blocks_.size();
  }

  // Determine previous block, either by using next's previous or the last block if there was no
  // next. If there are neither, they'll both remain null
  if (next) {
    previous = next->previous();
  } else if (!blocks_.isEmpty()) {
    previous = blocks_.last();
  }

  // Insert at index
  blocks_.insert(cache_index, block);
  block_array_indexes_.insert(cache_index, element);

  // Update previous/next
  if (previous) {
    previous->set_next(block);
    block->set_previous(previous);
  }

  if (next) {
    block->set_next(next);
    next->set_previous(block);
  }

  block->set_track(this);

  // Update ins/outs
  UpdateInOutFrom(cache_index);

  // Connect to the block
  connect(block, &Block::LengthChanged, this, &Track::BlockLengthChanged);

  // Invalidate cache now that block should have an in point
  InvalidateCache(TimeRange(block->in(), track_length()));

  // Emit block added signal
  emit BlockAdded(block);
}

void Track::BlockDisconnected(Node* node, int element)
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

  // Get cache index
  int cache_index = GetCacheIndexFromArrayIndex(element);

  // Remove block here
  blocks_.removeAt(cache_index);
  block_array_indexes_.removeAt(cache_index);

  emit BlockRemoved(b);

  // Update previous/nexts
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
  b->set_track(nullptr);

  // Update lengths
  if (next) {
    UpdateInOutFrom(blocks_.indexOf(next));
  } else if (blocks_.isEmpty()) {
    SetLengthInternal(rational());
  } else {
    SetLengthInternal(blocks_.last()->out());
  }

  disconnect(b, &Block::LengthChanged, this, &Track::BlockLengthChanged);

  InvalidateCache(invalidate_range);
}

void Track::BlockLengthChanged()
{
  // Assumes sender is a Block
  Block* b = static_cast<Block*>(sender());

  rational old_out = b->out();

  UpdateInOutFrom(blocks_.indexOf(b));

  rational new_out = b->out();

  TimeRange invalidate_region(qMin(old_out, new_out), track_length());

  InvalidateCache(invalidate_region);
}

void Track::MutedInputValueChanged()
{
  emit MutedChanged(IsMuted());
}

uint qHash(const Track::Reference &r, uint seed)
{
  // Not super efficient, but couldn't think of any better way to ensure a different hash each time
  return ::qHash(QStringLiteral("%1:%2").arg(QString::number(r.type()),
                                             QString::number(r.index())),
                 seed);
}

}
