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

#include "track.h"

#include <QApplication>
#include <QDebug>
#include <QFontMetrics>

#include "audio/audioprocessor.h"
#include "node/block/clip/clip.h"
#include "node/block/gap/gap.h"
#include "node/block/transition/transition.h"

namespace olive {

#define super Node

const double Track::kTrackHeightDefault = 3.0;
const double Track::kTrackHeightMinimum = 1.5;
const double Track::kTrackHeightInterval = 0.5;

const QString Track::kBlockInput = QStringLiteral("block_in");
const QString Track::kMutedInput = QStringLiteral("muted_in");

Track::Track() :
  track_type_(Track::kNone),
  index_(-1),
  locked_(false),
  sequence_(nullptr)
{
  AddInput(kBlockInput, NodeValue::kNone, InputFlags(kInputFlagArray | kInputFlagNotKeyframable | kInputFlagHidden));

  // Since blocks are time based, we can handle the invalidate timing a little more intelligently
  // on our end
  IgnoreInvalidationsFrom(kBlockInput);

  AddInput(kMutedInput, NodeValue::kBoolean, false, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  // Set default height
  track_height_ = kTrackHeightDefault;
}

void Track::set_type(const Type &track_type)
{
  track_type_ = track_type;
}

const Track::Type& Track::type() const
{
  return track_type_;
}

QString Track::Name() const
{
  if (track_type_ == Track::kVideo) {
    return tr("Video Track %1").arg(index_);
  } else if (track_type_ == Track::kAudio) {
    return tr("Audio Track %1").arg(index_);
  } else if (track_type_ == Track::kSubtitle) {
    return tr("Subtitle Track %1").arg(index_);
  }

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

Node::ActiveElements Track::GetActiveElementsAtTime(const QString &input, const TimeRange &r) const
{
  if (input == kBlockInput) {
    if (IsMuted() || blocks_.empty() || r.in() >= track_length() || r.out() <= 0) {
      return ActiveElements::kNoElements;
    } else {
      int start = GetBlockIndexAtTime(r.in());
      int end = GetBlockIndexAtTime(r.out());

      if (start == -1) {
        start = 0;
      }
      if (end == -1) {
        end = blocks_.size()-1;
      }

      ActiveElements a;
      for (int i=start; i<=end; i++) {
        Block *b = blocks_.at(i);
        if (b->is_enabled() && (dynamic_cast<ClipBlock*>(b) || dynamic_cast<TransitionBlock*>(b))) {
          a.add(GetArrayIndexFromCacheIndex(i));
        }
      }

      if (a.elements().empty()) {
        return ActiveElements::kNoElements;
      } else {
        return a;
      }
    }
  } else {
    return super::GetActiveElementsAtTime(input, r);
  }
}

void Track::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  if (this->type() == Track::kVideo) {
    // Just pass straight through
    NodeValueArray a = value[kBlockInput].toArray();
    if (!a.empty()) {
      table->Push(a.begin()->second);
    }
  } else if (this->type() == Track::kAudio) {
    // Audio
    ProcessAudioTrack(value, globals, table);
  }
}

TimeRange Track::InputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const
{
  if (input == kBlockInput && element >= 0) {
    int cache_index = GetCacheIndexFromArrayIndex(element);

    if (cache_index > -1) {
      return TransformRangeForBlock(blocks_.at(cache_index), input_time);
    }
  }

  return Node::InputTimeAdjustment(input, element, input_time);
}

TimeRange Track::OutputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const
{
  if (input == kBlockInput && element >= 0) {
    int cache_index = GetCacheIndexFromArrayIndex(element);

    if (cache_index > -1) {
      return TransformRangeFromBlock(blocks_.at(cache_index), input_time);
    }
  }

  return Node::OutputTimeAdjustment(input, element, input_time);
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

void Track::InputConnectedEvent(const QString &input, int element, Node *output)
{
  if (input == kBlockInput) {
    if (element == -1) {
      // User has replaced the entire array, we will invalidate everything
      InvalidateAll(kBlockInput, element);
      return;
    }

    // Check if a block was connected, if not, ignore
    Block* block = dynamic_cast<Block*>(output);

    if (!block) {
      return;
    }

    // Determine where in the cache this block will be
    int cache_index = -1;
    Block *previous = nullptr, *next = nullptr;

    int arr_sz = InputArraySize(kBlockInput);
    for (int i=element+1; i<arr_sz; i++) {
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
    Node::InvalidateCache(TimeRange(block->in(), track_length()), kBlockInput);

    // Emit block added signal
    emit BlockAdded(block);
  }
}

void Track::InputDisconnectedEvent(const QString &input, int element, Node *output)
{
  if (input == kBlockInput) {
    if (element == -1) {
      // User has replaced the entire array, we will invalidate everything
      InvalidateAll(kBlockInput, element);
      return;
    }

    Block* b = dynamic_cast<Block*>(output);

    if (!b) {
      return;
    }

    emit BlockRemoved(b);

    TimeRange invalidate_range(b->in(), track_length());

    // Get cache index
    int cache_index = GetCacheIndexFromArrayIndex(element);

    // Remove block here
    blocks_.removeAt(cache_index);
    block_array_indexes_.removeAt(cache_index);

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
    }
    emit TrackLengthChanged();

    disconnect(b, &Block::LengthChanged, this, &Track::BlockLengthChanged);

    Node::InvalidateCache(invalidate_range, kBlockInput);
  }
}

void Track::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element)

  if (input == kMutedInput) {
    emit MutedChanged(IsMuted());
  }
}

void Track::Retranslate()
{
  super::Retranslate();

  SetInputName(kBlockInput, tr("Blocks"));
  SetInputName(kMutedInput, tr("Muted"));
}

void Track::SetIndex(const int &index)
{
  int old = index_;

  index_ = index;

  emit IndexChanged(old, index_);
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
    if (block->in() == time) {
      break;
    }

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

void Track::InvalidateCache(const TimeRange& range, const QString& from, int element, InvalidateCacheOptions options)
{
  TimeRange limited;

  const Block* b;

  if (from == kBlockInput
      && element >= 0
      && (b = dynamic_cast<const Block*>(GetConnectedOutput(from, element)))
      && !options.value(QStringLiteral("lengthevent")).toBool()) {
    // Limit the range signal to the corresponding block
    TimeRange transformed = TransformRangeFromBlock(b, range);

    if (transformed.out() <= b->in() || transformed.in() >= b->out()) {
      return;
    }

    limited = TimeRange(qMax(transformed.in(), b->in()), qMin(transformed.out(), b->out()));
  } else {
    limited = range;
  }

  // NOTE: For now, I figure we drop this key, but we may find in the future that it's advantageous
  //       to keep it
  options.remove(QStringLiteral("lengthevent"));

  Node::InvalidateCache(limited, from, element, options);
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
  InputArrayPrepend(kBlockInput);
  Node::ConnectEdge(block, NodeInput(this, kBlockInput, 0));

  // Everything has shifted at this point
  Node::InvalidateCache(TimeRange(0, track_length()), kBlockInput);
}

void Track::InsertBlockAtIndex(Block *block, int index)
{
  int insert_index = GetArrayIndexFromCacheIndex(index);
  InputArrayInsert(kBlockInput, insert_index);
  Node::ConnectEdge(block, NodeInput(this, kBlockInput, insert_index));

  Node::InvalidateCache(TimeRange(block->in(), track_length()), kBlockInput);
}

void Track::AppendBlock(Block *block)
{
  InputArrayAppend(kBlockInput);
  Node::ConnectEdge(block, NodeInput(this, kBlockInput, InputArraySize(kBlockInput) - 1));

  // Invalidate area that block was added to
  Node::InvalidateCache(TimeRange(block->in(), block->out()), kBlockInput);
}

void Track::RippleRemoveBlock(Block *block)
{
  rational remove_in = block->in();
  rational remove_out = block->out();

  InputArrayRemove(kBlockInput, GetArrayIndexFromBlock(block));

  Node::InvalidateCache(TimeRange(remove_in, qMax(track_length(), remove_out)), kBlockInput);
}

void Track::ReplaceBlock(Block *old, Block *replace)
{
  int index_of_old_block = GetArrayIndexFromBlock(old);

  DisconnectEdge(old, NodeInput(this, kBlockInput, index_of_old_block));

  ConnectEdge(replace, NodeInput(this, kBlockInput, index_of_old_block));

  if (old->length() == replace->length()) {
    Node::InvalidateCache(TimeRange(replace->in(), replace->out()), kBlockInput);
  } else {
    Node::InvalidateCache(TimeRange(replace->in(), track_length()), kBlockInput);
  }
}

rational Track::track_length() const
{
  if (blocks_.isEmpty()) {
    return 0;
  } else {
    return blocks_.last()->out();
  }
}

bool Track::IsMuted() const
{
  return GetStandardValue(kMutedInput).toBool();
}

bool Track::IsLocked() const
{
  return locked_;
}

void Track::SetMuted(bool e)
{
  SetStandardValue(kMutedInput, e);
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
  emit TrackLengthChanged();
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

int Track::GetBlockIndexAtTime(const rational &time) const
{
  if (time < 0 || time >= track_length()) {
    return -1;
  }

  // Use binary search to find block at time
  int low = 0;
  int high = blocks_.size() - 1;
  while (low <= high) {
    int mid = low + (high - low) / 2;

    Block* block = blocks_.at(mid);
    if (block->in() <= time && block->out() > time) {
      return mid;
    } else if (block->out() <= time) {
      low = mid + 1;
    } else {
      high = mid - 1;
    }
  }

  return -1;
}

void Track::ProcessAudioTrack(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  const TimeRange &range = globals.time();

  // All these blocks will need to output to a buffer so we create one here
  SampleBuffer block_range_buffer(globals.aparams(), range.length());
  block_range_buffer.silence();

  // Loop through active blocks retrieving their audio
  NodeValueArray arr = value[kBlockInput].toArray();

  for (auto it=arr.cbegin(); it!=arr.cend(); it++) {
    Block *b = blocks_.at(GetCacheIndexFromArrayIndex(it->first));

    TimeRange range_for_block(qMax(b->in(), range.in()),
                              qMin(b->out(), range.out()));

    qint64 destination_offset = globals.aparams().time_to_samples(range_for_block.in() - range.in());
    qint64 max_dest_sz = globals.aparams().time_to_samples(range_for_block.length());

    // Destination buffer
    SampleBuffer samples_from_this_block = it->second.toSamples();
    ClipBlock *clip_cast = dynamic_cast<ClipBlock*>(b);

    if (samples_from_this_block.is_allocated()) {
      // If this is a clip, we might have extra speed/reverse information
      if (clip_cast) {
        double speed_value = clip_cast->speed();
        bool reversed = clip_cast->reverse();

        if (qIsNull(speed_value)) {
          // Just silence, don't think there's any other practical application of 0 speed audio
          samples_from_this_block.silence();
        } else if (!qFuzzyCompare(speed_value, 1.0)) {
          if (clip_cast->maintain_audio_pitch()) {
            AudioProcessor processor;

            if (processor.Open(samples_from_this_block.audio_params(), samples_from_this_block.audio_params(), speed_value)) {
              AudioProcessor::Buffer out;

              // FIXME: This is not the best way to do this, the TempoProcessor works best
              //        when it's given a continuous stream of audio, which is challenging
              //        in our current "modular" audio system. This should still work reasonably
              //        well on export (assuming audio is all generated at once on export), but
              //        users may hear clicks and pops in the audio during preview due to this
              //        approach.
              int r = processor.Convert(samples_from_this_block.to_raw_ptrs().data(), samples_from_this_block.sample_count(), nullptr);

              if (r < 0) {
                qCritical() << "Failed to change tempo of audio:" << r;
              } else {
                processor.Flush();

                processor.Convert(nullptr, 0, &out);

                if (!out.empty()) {
                  int nb_samples = out.front().size() * samples_from_this_block.audio_params().bytes_per_sample_per_channel();

                  if (nb_samples) {
                    SampleBuffer new_samples(samples_from_this_block.audio_params(), nb_samples);

                    for (int i=0; i<out.size(); i++) {
                      memcpy(new_samples.data(i), out[i].data(), out[i].size());
                    }

                    samples_from_this_block = new_samples;
                  }
                }
              }
            }
          } else {
            // Multiply time
            samples_from_this_block.speed(speed_value);
          }
        }

        if (reversed) {
          samples_from_this_block.reverse();
        }
      }

      qint64 copy_length = qMin(max_dest_sz, qint64(samples_from_this_block.sample_count()));

      // Copy samples into destination buffer
      for (int i=0; i<samples_from_this_block.audio_params().channel_count(); i++) {
        block_range_buffer.set(i, samples_from_this_block.data(i), destination_offset, copy_length);
      }
    }
  }

  table->Push(NodeValue::kSamples, QVariant::fromValue(block_range_buffer), this);
}

void Track::BlockLengthChanged()
{
  // Assumes sender is a Block
  Block* b = static_cast<Block*>(sender());

  UpdateInOutFrom(blocks_.indexOf(b));
}

uint qHash(const Track::Reference &r, uint seed)
{
  // Not super efficient, but couldn't think of any better way to ensure a different hash each time
  return ::qHash(QStringLiteral("%1:%2").arg(QString::number(r.type()),
                                             QString::number(r.index())),
                 seed);
}

QDataStream &operator<<(QDataStream &out, const Track::Reference &ref)
{
  out << static_cast<int>(ref.type()) << ref.index();

  return out;
}

QDataStream &operator>>(QDataStream &in, Track::Reference &ref)
{
  int type;
  int index;

  in >> type >> index;

  ref = Track::Reference(static_cast<Track::Type>(type), index);

  return in;
}

}
