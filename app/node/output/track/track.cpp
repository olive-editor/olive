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
const QString Track::kArrayMapInput = QStringLiteral("arraymap_in");

Track::Track() :
  track_type_(Track::kNone),
  index_(-1),
  locked_(false),
  sequence_(nullptr),
  ignore_arraymap_(0),
  arraymap_invalid_(false),
  ignore_arraymap_set_(false)
{
  AddInput(kBlockInput, NodeValue::kNone, InputFlags(kInputFlagArray | kInputFlagNotKeyframable | kInputFlagHidden | kInputFlagIgnoreInvalidations));

  AddInput(kMutedInput, NodeValue::kBoolean, false, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  AddInput(kArrayMapInput, NodeValue::kBinary, InputFlags(kInputFlagStatic | kInputFlagHidden | kInputFlagIgnoreInvalidations));

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
  return tr("Node for representing and processing a single array of Blocks sorted by time. Also represents the end of a Sequence.");
}

void ProcessAudio(const void *context, const SampleJob &job, SampleBuffer &block_range_buffer)
{
  const QVector<Block*> *blocks = static_cast<const QVector<Block*> *>(context);

  const ValueParams &p = job.value_params();
  const TimeRange &range = job.value_params().time();

  // All these blocks will need to output to a buffer so we create one here
  block_range_buffer.silence();

  for (auto it = job.GetValues().cbegin(); it != job.GetValues().cend(); it++) {
    Block *b = blocks->at(it.key().toInt());

    TimeRange range_for_block(qMax(b->in(), range.in()),
                              qMin(b->out(), range.out()));

    qint64 source_offset = 0;
    qint64 destination_offset = p.aparams().time_to_samples(range_for_block.in() - range.in());
    qint64 max_dest_sz = p.aparams().time_to_samples(range_for_block.length());

    // Destination buffer
    SampleBuffer samples_from_this_block = it.value().toSamples();

    if (samples_from_this_block.is_allocated()) {
      // If this is a clip, we might have extra speed/reverse information
      if (ClipBlock *clip_cast = dynamic_cast<ClipBlock*>(b)) {
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

      qint64 copy_length = qMin(max_dest_sz, qint64(samples_from_this_block.sample_count() - source_offset));

      // Copy samples into destination buffer
      for (int i=0; i<samples_from_this_block.audio_params().channel_count(); i++) {
        block_range_buffer.set(i, samples_from_this_block.data(i) + source_offset, destination_offset, copy_length);
      }
    }
  }
}

NodeValue Track::Value(const ValueParams &p) const
{
  if (!IsMuted() && !blocks_.empty() && p.time().in() < track_length() && p.time().out() > 0) {
    if (this->type() == Track::kVideo) {
      // Just pass straight through
      int index = GetBlockIndexAtTime(p.time().in());
      if (index != -1) {
        Block *b = blocks_.at(index);

        if (b->is_enabled() && (dynamic_cast<ClipBlock*>(b) || dynamic_cast<TransitionBlock*>(b))) {
          return GetInputValue(p, kBlockInput, GetArrayIndexFromCacheIndex(index));
        }
      }
    } else if (this->type() == Track::kAudio) {
      // Audio
      const TimeRange &range = p.time();

      int start = GetBlockIndexAtTime(range.in());
      int end = GetBlockIndexAtTime(range.out());

      if (start == -1) {
        start = 0;
      }
      if (end == -1) {
        end = blocks_.size()-1;
      }

      if (blocks_.at(end)->in() == range.out()) {
        end--;
      }

      SampleJob job(p);

      job.set_function(ProcessAudio, &blocks_);

      for (int i=start; i<=end; i++) {
        Block *b = blocks_.at(i);
        if (b->is_enabled() && (dynamic_cast<ClipBlock*>(b) || dynamic_cast<TransitionBlock*>(b))) {
          job.Insert(QString::number(i), GetInputValue(p, kBlockInput, GetArrayIndexFromCacheIndex(i)));
        }
      }

      return job;
    }
  }

  return NodeValue();
}

TimeRange Track::InputTimeAdjustment(const QString& input, int element, const TimeRange& input_time, bool clamp) const
{
  if (input == kBlockInput && element >= 0) {
    int cache_index = GetCacheIndexFromArrayIndex(element);

    if (cache_index > -1) {
      TimeRange r = input_time;
      Block *b = blocks_.at(cache_index);

      if (clamp) {
        r.set_range(std::max(r.in(), b->in()), std::min(r.out(), b->out()));
      }

      return TransformRangeForBlock(b, r);
    }
  }

  return Node::InputTimeAdjustment(input, element, input_time, clamp);
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
  emit TrackHeightChanged(track_height_);
}

bool Track::LoadCustom(QXmlStreamReader *reader, SerializedData *data)
{
  ignore_arraymap_set_ = true;

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("height")) {
      this->SetTrackHeight(reader->readElementText().toDouble());
    } else {
      reader->skipCurrentElement();
    }
  }

  return true;
}

void Track::SaveCustom(QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("height"), QString::number(this->GetTrackHeight()));
}

void Track::PostLoadEvent(SerializedData *data)
{
  ignore_arraymap_set_ = false;
  RefreshBlockCacheFromArrayMap();
}

void Track::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element)

  if (input == kMutedInput) {
    emit MutedChanged(IsMuted());
  } else if (input == kArrayMapInput) {
    if (ignore_arraymap_ > 0) {
      ignore_arraymap_--;
    } else {
      RefreshBlockCacheFromArrayMap();
    }
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

bool Track::IsRangeFree(const TimeRange &range) const
{
  Block *b = NearestBlockBeforeOrAt(range.in());
  if (!b) {
    // No block here, assume track is empty here
    return true;
  }

  if (!dynamic_cast<GapBlock*>(b)) {
    // There's a block at or around the start point that isn't a gap, range is not free
    return false;
  }

  while ((b = b->next())) {
    if (b->in() >= range.out()) {
      // This block is after the range, no longer relevant
      break;
    } else if (!dynamic_cast<GapBlock*>(b)) {
      // Found a block in this range, range is not free
      return false;
    }
  }

  // If we get here, we couldn't find anything in the way of this range
  return true;
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

    InsertBlockAtIndex(block, before_index + 1);
  }
}

void Track::PrependBlock(Block *block)
{
  InsertBlockAtIndex(block, 0);
}

void Track::InsertBlockAtIndex(Block *block, int index)
{
  // Set track
  Q_ASSERT(block->track() == nullptr);
  block->set_track(this);

  // Update array
  int array_index = ConnectBlock(block);
  blocks_.insert(index, block);
  block_array_indexes_.insert(index, array_index);

  // Handle previous/next
  Block *previous = (index > 0) ? blocks_.at(index - 1) : nullptr;
  Block *next = (index < blocks_.size()-1) ? blocks_.at(index + 1) : nullptr ;
  Block::set_previous_next(previous, block);
  Block::set_previous_next(block, next);

  // Update in/out
  UpdateInOutFrom(index);

  connect(block, &Block::LengthChanged, this, &Track::BlockLengthChanged);

  Node::InvalidateCache(TimeRange(block->in(), track_length()), kBlockInput);

  emit BlockAdded(block);

  UpdateArrayMap();
}

void Track::AppendBlock(Block *block)
{
  InsertBlockAtIndex(block, blocks_.size());
}

void Track::RippleRemoveBlock(Block *block)
{
  rational remove_in = block->in();
  rational remove_out = block->out();

  emit BlockRemoved(block);

  // Set track
  Q_ASSERT(block->track() == this);
  block->set_track(nullptr);

  // Update array
  int index = blocks_.indexOf(block);
  Q_ASSERT(index != -1);

  int array_index = block_array_indexes_.at(index);

  blocks_.removeAt(index);
  block_array_indexes_.removeAt(index);

  Node::DisconnectEdge(block, NodeInput(this, kBlockInput, array_index));
  empty_inputs_.push_back(array_index);
  disconnect(block, &Block::LengthChanged, this, &Track::BlockLengthChanged);

  // Handle previous/next
  Block *previous = (index > 0) ? blocks_.at(index - 1) : nullptr;
  Block *next = (index < blocks_.size()) ? blocks_.at(index) : nullptr;
  Block::set_previous_next(previous, next);
  block->set_previous(nullptr);
  block->set_next(nullptr);
  block->set_in(0);
  block->set_out(block->length());

  // Update in/outs
  UpdateInOutFrom(index);

  Node::InvalidateCache(TimeRange(remove_in, qMax(track_length(), remove_out)), kBlockInput);

  UpdateArrayMap();
}

void Track::ReplaceBlock(Block *old, Block *replace)
{
  emit BlockRemoved(old);

  // Set track
  Q_ASSERT(old->track() == this);
  old->set_track(nullptr);

  Q_ASSERT(replace->track() == nullptr);
  replace->set_track(this);

  // Update array
  int cache_index = blocks_.indexOf(old);
  int index_of_old_block = GetArrayIndexFromCacheIndex(cache_index);

  DisconnectEdge(old, NodeInput(this, kBlockInput, index_of_old_block));
  ConnectEdge(replace, NodeInput(this, kBlockInput, index_of_old_block));
  blocks_.replace(cache_index, replace);
  disconnect(old, &Block::LengthChanged, this, &Track::BlockLengthChanged);
  connect(replace, &Block::LengthChanged, this, &Track::BlockLengthChanged);

  // Handle previous/next
  replace->set_previous(old->previous());
  replace->set_next(old->next());
  old->set_previous(nullptr);
  old->set_next(nullptr);
  if (replace->previous()) {
    replace->previous()->set_next(replace);
  }
  if (replace->next()) {
    replace->next()->set_previous(replace);
  }

  if (old->length() == replace->length()) {
    replace->set_in(replace->previous() ? replace->previous()->out() : 0);
    replace->set_out(replace->in() + replace->length());

    Node::InvalidateCache(TimeRange(replace->in(), replace->out()), kBlockInput);
  } else {
    // Update in/outs
    UpdateInOutFrom(cache_index);

    Node::InvalidateCache(TimeRange(replace->in(), track_length()), kBlockInput);
  }

  emit BlockAdded(replace);

  UpdateArrayMap();
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

void Track::InputConnectedEvent(const QString &input, int element, Node *node)
{
  if (arraymap_invalid_ && input == kBlockInput && element >= 0) {
    RefreshBlockCacheFromArrayMap();
  }
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

int Track::ConnectBlock(Block *b)
{
  if (!empty_inputs_.empty()) {
    int index = empty_inputs_.front();
    empty_inputs_.pop_front();

    Node::ConnectEdge(b, NodeInput(this, kBlockInput, index));

    return index;
  } else {
    int old_sz = InputArraySize(kBlockInput);
    InputArrayAppend(kBlockInput);
    Node::ConnectEdge(b, NodeInput(this, kBlockInput, old_sz));
    return old_sz;
  }
}

void Track::UpdateArrayMap()
{
  ignore_arraymap_++;
  SetStandardValue(kArrayMapInput, QByteArray(reinterpret_cast<const char *>(block_array_indexes_.data()), block_array_indexes_.size() * sizeof(uint32_t)));
}

void Track::RefreshBlockCacheFromArrayMap()
{
  if (ignore_arraymap_set_) {
    return;
  }

  // Disconnecting any existing blocks
  for (Block *b : qAsConst(blocks_)) {
    Q_ASSERT(b->track() == this);
    b->set_track(nullptr);
    b->set_previous(nullptr);
    b->set_next(nullptr);
    b->set_in(0);
    b->set_out(b->length());
    disconnect(b, &Block::LengthChanged, this, &Track::BlockLengthChanged);
  }

  QByteArray bytes = GetStandardValue(kArrayMapInput).toByteArray();
  block_array_indexes_.resize(bytes.size() / sizeof(uint32_t));
  memcpy(block_array_indexes_.data(), bytes.data(), bytes.size());
  blocks_.clear();
  blocks_.reserve(block_array_indexes_.size());

  Block *prev = nullptr;
  arraymap_invalid_ = false;

  for (int i = 0; i < block_array_indexes_.size(); i++) {
    Block *b = static_cast<Block*>(GetConnectedOutput(kBlockInput, block_array_indexes_.at(i)));

    Block::set_previous_next(prev, b);

    if (b) {
      b->set_track(this);
      connect(b, &Block::LengthChanged, this, &Track::BlockLengthChanged);

      blocks_.append(b);
      prev = b;
    } else {
      block_array_indexes_.resize(i);
      arraymap_invalid_ = true;
      break;
    }
  }

  if (prev) {
    prev->set_next(nullptr);
  }

  UpdateInOutFrom(0);
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
