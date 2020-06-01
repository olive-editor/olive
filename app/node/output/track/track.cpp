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

OLIVE_NAMESPACE_ENTER

TrackOutput::TrackOutput() :
  track_type_(Timeline::kTrackTypeNone),
  block_invalidate_cache_stack_(0),
  index_(-1),
  locked_(false),
  queued_length_change_(false)
{
  block_input_ = new NodeInputArray("block_in", NodeParam::kAny);
  block_input_->set_is_keyframable(false);
  AddInput(block_input_);
  connect(block_input_, &NodeInputArray::SubParamEdgeAdded, this, &TrackOutput::BlockConnected);
  connect(block_input_, &NodeInputArray::SubParamEdgeRemoved, this, &TrackOutput::BlockDisconnected);
  disconnect(block_input_, &NodeInputArray::SubParamEdgeAdded, this, &TrackOutput::InputConnectionChanged);
  disconnect(block_input_, &NodeInputArray::SubParamEdgeRemoved, this, &TrackOutput::InputConnectionChanged);

  muted_input_ = new NodeInput("muted_in", NodeParam::kBoolean);
  muted_input_->set_is_keyframable(false);
  connect(muted_input_, &NodeInput::ValueChanged, this, &TrackOutput::MutedInputValueChanged);
  AddInput(muted_input_);

  // Set default height
  track_height_ = GetDefaultTrackHeight();
}

void TrackOutput::set_track_type(const Timeline::TrackType &track_type)
{
  track_type_ = track_type;
}

const Timeline::TrackType& TrackOutput::track_type() const
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
  return QStringLiteral("org.olivevideoeditor.Olive.track");
}

QList<Node::CategoryID> TrackOutput::Category() const
{
  return {kCategoryTimeline};
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
  Block::Retranslate();

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

QList<Block *> TrackOutput::BlocksAtTimeRange(const TimeRange &range) const
{
  QList<Block*> list;

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

const QList<Block *> &TrackOutput::Blocks() const
{
  return block_cache_;
}

void TrackOutput::InvalidateCache(const TimeRange &range, NodeInput *from, NodeInput *source)
{
  if (block_invalidate_cache_stack_ == 0) {
    PushLengthChangeSignal();

    Node::InvalidateCache(TimeRange(qMax(range.in(), rational(0)), qMin(range.out(), track_length())), from, source);
  }
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
  BlockInvalidateCache();

  block_input_->Prepend();
  NodeParam::ConnectEdge(block->output(), block_input_->First());

  UnblockInvalidateCache();

  // Everything has shifted at this point
  InvalidateCache(TimeRange(0, track_length()), block_input_, block_input_);
}

void TrackOutput::InsertBlockAtIndex(Block *block, int index)
{
  BlockInvalidateCache();

  int insert_index = GetInputIndexFromCacheIndex(index);
  block_input_->InsertAt(insert_index);
  NodeParam::ConnectEdge(block->output(),
                         block_input_->At(insert_index));

  UnblockInvalidateCache();

  InvalidateCache(TimeRange(block->in(), track_length()), block_input_, block_input_);
}

void TrackOutput::AppendBlock(Block *block)
{
  BlockInvalidateCache();

  block_input_->Append();
  NodeParam::ConnectEdge(block->output(), block_input_->Last());

  UnblockInvalidateCache();

  // Invalidate area that block was added to
  InvalidateCache(TimeRange(block->in(), track_length()), block_input_, block_input_);
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

  block_input_->RemoveAt(GetInputIndexFromCacheIndex(block));

  UnblockInvalidateCache();

  InvalidateCache(TimeRange(remove_in, track_length()), block_input_, block_input_);
}

void TrackOutput::ReplaceBlock(Block *old, Block *replace)
{
  BlockInvalidateCache();

  int index_of_old_block = GetInputIndexFromCacheIndex(old);

  NodeParam::DisconnectEdge(old->output(),
                            block_input_->At(index_of_old_block));

  NodeParam::ConnectEdge(replace->output(),
                         block_input_->At(index_of_old_block));

  UnblockInvalidateCache();

  if (old->length() == replace->length()) {
    InvalidateCache(TimeRange(replace->in(), replace->out()), block_input_, block_input_);
  } else {
    InvalidateCache(TimeRange(replace->in(), RATIONAL_MAX), block_input_, block_input_);
  }
}

TrackOutput *TrackOutput::TrackFromBlock(const Block *block)
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

NodeInputArray *TrackOutput::block_input() const
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

void TrackOutput::PushLengthChangeSignal()
{
  if (queued_length_change_) {
    queued_length_change_ = false;
    SetLengthInternal(queued_length_);
  }
}

void TrackOutput::SetTrackName(const QString &name)
{
  track_name_ = name;
}

void TrackOutput::SetMuted(bool e)
{
  muted_input_->set_standard_value(e);
  InvalidateCache(TimeRange(0, track_length()), block_input_, block_input_);
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
  for (int i=0; i<block_input_->GetSize(); i++) {
    if (block_input_->At(i)->get_connected_node() == block) {
      return i;
    }
  }

  return -1;
}

void TrackOutput::SetLengthInternal(const rational &r)
{
  if (block_invalidate_cache_stack_ > 0) {
    queued_length_change_ = true;
  }

  if (queued_length_change_) {
    queued_length_ = r;
    return;
  }

  if (r != track_length_) {
    TimeRange invalidate_range(track_length_, r);

    track_length_ = r;
    emit TrackLengthChanged();

    InvalidateCache(invalidate_range,
                    block_input_,
                    block_input_);
  }
}

void TrackOutput::BlockConnected(NodeEdgePtr edge)
{
  QList<Block*> new_block_list;

  foreach (NodeInput* i, block_input_->sub_params()) {
    Node* connected = i->get_connected_node();

    if (connected
        && connected->IsBlock()
        && !new_block_list.contains(static_cast<Block*>(connected))) {
      Block* b = static_cast<Block*>(connected);

      if (!new_block_list.isEmpty()) {
        new_block_list.last()->set_next(b);
        b->set_previous(new_block_list.last());
      }

      new_block_list.append(b);

      if (!block_cache_.contains(b)) {
        // Make connections to this block
        connect(b, &Block::LengthChanged, this, &TrackOutput::BlockLengthChanged);

        emit BlockAdded(b);
      }
    }
  }

  if (!new_block_list.isEmpty()) {
    new_block_list.first()->set_previous(nullptr);
    new_block_list.last()->set_next(nullptr);
  }

  int new_index;

  for (new_index = 0; new_index < block_cache_.size(); new_index++) {
    if (block_cache_.at(new_index) != new_block_list.at(new_index)) {
      break;
    }
  }

  block_cache_ = new_block_list;

  UpdateInOutFrom(new_index);

  InputConnectionChanged(edge);

  /*
  // Determine what node was just connected
  Node* connected_node = edge->output()->parentNode();

  // If this node is a block, we can do something with it
  if (connected_node->IsBlock()) {
    Block* connected_block = static_cast<Block*>(connected_node);

    // See where this input falls in our internal "block cache"
    Block* next = nullptr;
    for (int i=block_input_->IndexOfSubParameter(edge->input())+1; i<block_input_->GetSize(); i++) {
      Node* that_node = block_input_->At(i)->get_connected_node();

      // If we find a block, this is the block that will follow the one just connected
      if (that_node && that_node->IsBlock()) {
        next = static_cast<Block*>(that_node);
        break;
      }
    }

    int real_block_index;

    // Either insert or append depending on if we found a "next" block
    if (next) {
      // Insert block before this next block
      real_block_index = block_cache_.indexOf(next);
      block_cache_.insert(real_block_index, connected_block);

      // Update values with next
      next->set_previous(connected_block);
      connected_block->set_next(next);
    } else {
      // No "next", this block must come at the end
      real_block_index = block_cache_.size();
      block_cache_.append(connected_block);

      // Update next value
      connected_block->set_next(nullptr);
    }

    // For all blocks after the block we inserted (including it), update the "previous" and "next"
    // fields as well as the in/out values
    if (real_block_index == 0) {
      connected_block->set_previous(nullptr);
    } else {
      Block* prev = block_cache_.at(real_block_index - 1);

      connected_block->set_previous(prev);
      prev->set_next(connected_block);
    }

    UpdateInOutFrom(real_block_index);

    // Make connections to this block
    connect(connected_block, &Block::LengthChanged, this, &TrackOutput::BlockLengthChanged);

    emit BlockAdded(connected_block);
  }

  InputConnectionChanged(edge);
  */
}

void TrackOutput::BlockDisconnected(NodeEdgePtr edge)
{
  Block* b = static_cast<Block*>(edge->output()->parentNode());

  if (block_cache_.contains(b)) {
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
  }

  /*
  // See what kind of node was just connected
  Node* connected_node = edge->output()->parentNode();

  // If this was a block, we would have put it in our block cache in BlockConnected()
  int index_of_block = block_cache_.indexOf(static_cast<Block*>(connected_node));
  if (index_of_block > -1) {
    Block* connected_block = static_cast<Block*>(connected_node);

    // Determine what index this block was in our cache and remove it
    block_cache_.removeAt(index_of_block);

    // If there were blocks following this one, update their ins/outs
    UpdateInOutFrom(index_of_block);

    // Join the previous and next blocks together
    if (connected_block->previous()) {
      connected_block->previous()->set_next(connected_block->next());
    }

    if (connected_block->next()) {
      connected_block->next()->set_previous(connected_block->previous());
    }

    disconnect(connected_block, &Block::LengthChanged, this, &TrackOutput::BlockLengthChanged);

    emit BlockRemoved(connected_block);
  }
  */

  InputConnectionChanged(edge);
}

void TrackOutput::BlockLengthChanged()
{
  // Assumes sender is a Block
  Block* b = static_cast<Block*>(sender());

  rational old_out = b->out();

  UpdateInOutFrom(block_cache_.indexOf(b));

  rational new_out = b->out();

  TimeRange invalidate_region(qMin(old_out, new_out), track_length());

  InvalidateCache(invalidate_region, block_input_, block_input_);
}

void TrackOutput::MutedInputValueChanged()
{
  emit MutedChanged(IsMuted());
}

OLIVE_NAMESPACE_EXIT
